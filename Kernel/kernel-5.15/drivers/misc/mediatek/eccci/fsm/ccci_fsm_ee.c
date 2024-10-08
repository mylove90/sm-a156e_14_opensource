// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2016 MediaTek Inc.
 */

#include "ccci_fsm_internal.h"
#include "modem_sys.h"
#include "md_sys1_platform.h"
#if IS_ENABLED(CONFIG_MTK_AEE_FEATURE)
#include <mt-plat/aee.h>
#endif

void mdee_set_ex_start_str(struct ccci_fsm_ee *ee_ctl,
	const unsigned int type, const char *str)
{
	u64 ts_nsec;
	unsigned long rem_nsec;
	int ret = 0;

	if (type == MD_FORCE_ASSERT_BY_AP_MPU) {
		ret = scnprintf(ee_ctl->ex_mpu_string, MD_EX_MPU_STR_LEN,
			"EMI MPU VIOLATION: %s", str);
		if (ret <= 0 || ret >= MD_EX_MPU_STR_LEN) {
			CCCI_ERROR_LOG(0, FSM,
				"%s:snprintf ee_ctl->ex_mpu_string fail\n",
				__func__);
			ee_ctl->ex_mpu_string[0] = 0;
		}
	}
	ts_nsec = local_clock();
	rem_nsec = do_div(ts_nsec, 1000000000);
	scnprintf(ee_ctl->ex_start_time, MD_EX_START_TIME_LEN,
		"AP detect MDEE time:%5lu.%06lu\n",
		(unsigned long)ts_nsec, rem_nsec / 1000);
	CCCI_MEM_LOG_TAG(0, FSM, "%s\n",
		ee_ctl->ex_start_time);
}

void fsm_md_bootup_timeout_handler(struct ccci_fsm_ee *ee_ctl)
{
	struct ccci_mem_layout *mem_layout
		= ccci_md_get_mem();

	CCCI_NORMAL_LOG(0, FSM,
		"Dump MD layout struct\n");
	ccci_util_mem_dump(CCCI_DUMP_MEM_DUMP, mem_layout,
		sizeof(struct ccci_mem_layout));
	CCCI_NORMAL_LOG(0, FSM,
		"Dump queue 0 & 1\n");
	ccci_md_dump_info(
		(DUMP_FLAG_QUEUE_0_1 | DUMP_MD_BOOTUP_STATUS
		| DUMP_FLAG_REG | DUMP_FLAG_CCIF_REG), NULL, 0);
	CCCI_NORMAL_LOG(0, FSM,
		"Dump MD ee boot failed info\n");

	ee_ctl->ops->dump_ee_info(ee_ctl, MDEE_DUMP_LEVEL_BOOT_FAIL, 0);
}

void fsm_md_exception_stage(struct ccci_fsm_ee *ee_ctl, int stage)
{
	unsigned long flags;

	if (stage == 0) { /* CCIF handshake just came in */
		mdee_set_ex_start_str(ee_ctl, 0, NULL);
		ee_ctl->mdlog_dump_done = 0;
		ee_ctl->ee_info_flag = 0;
		spin_lock_irqsave(&ee_ctl->ctrl_lock, flags);
		ee_ctl->ee_info_flag |= (MD_EE_FLOW_START | MD_EE_SWINT_GET);
		if (ccci_fsm_get_md_state()
				== BOOT_WAITING_FOR_HS1)
			ee_ctl->ee_info_flag |= MD_EE_DUMP_IN_GPD;
		spin_unlock_irqrestore(&ee_ctl->ctrl_lock, flags);
	} else if (stage == 1) { /* got MD_EX_REC_OK or first timeout */
		int ee_case;
		unsigned int ee_info_flag = 0;
		unsigned int md_dump_flag = 0;
		struct ccci_mem_layout *mem_layout = ccci_md_get_mem();
		struct ccci_smem_region *mdss_dbg
			= ccci_md_get_smem_by_user_id(
				SMEM_USER_RAW_MDSS_DBG);
		struct ccci_modem *md = ccci_get_modem();

		CCCI_ERROR_LOG(0, FSM, "MD exception stage 1!\n");
#if IS_ENABLED(CONFIG_MTK_AEE_FEATURE)
		tracing_off();
#endif
		CCCI_MEM_LOG_TAG(0, FSM,
			"MD exception stage 1! ee=%x\n",
			ee_ctl->ee_info_flag);
		spin_lock_irqsave(&ee_ctl->ctrl_lock, flags);
		ee_info_flag = ee_ctl->ee_info_flag;
		spin_unlock_irqrestore(&ee_ctl->ctrl_lock, flags);

		if ((ee_info_flag & (MD_EE_SWINT_GET
			| MD_EE_MSG_GET | MD_EE_OK_MSG_GET)) ==
		    (MD_EE_SWINT_GET | MD_EE_MSG_GET
			| MD_EE_OK_MSG_GET)) {
			ee_case = MD_EE_CASE_NORMAL;
			CCCI_DEBUG_LOG(0, FSM,
				"Recv SWINT & MD_EX & MD_EX_REC_OK\n");
		} else if (ee_info_flag & MD_EE_MSG_GET) {
			ee_case = MD_EE_CASE_ONLY_EX;
			CCCI_NORMAL_LOG(0, FSM, "Only recv MD_EX.\n");
		} else if (ee_info_flag & MD_EE_SWINT_GET) {
			ee_case = MD_EE_CASE_ONLY_SWINT;
			CCCI_NORMAL_LOG(0, FSM, "Only recv SWINT.\n");
		} else if (ee_info_flag & MD_EE_PENDING_TOO_LONG) {
			ee_case = MD_EE_CASE_NO_RESPONSE;
		} else if (ee_info_flag & MD_EE_WDT_GET) {
			ee_case = MD_EE_CASE_WDT;
			md->per_md_data.md_dbg_dump_flag |=
			(1 << MD_DBG_DUMP_TOPSM) | (1 << MD_DBG_DUMP_MDRGU)
			| (1 << MD_DBG_DUMP_OST);
		} else {
			CCCI_ERROR_LOG(0, FSM,
				"Invalid MD_EX, ee_info=%x\n", ee_info_flag);
			goto _dump_done;
		}
		ee_ctl->ee_case = ee_case;

		/* Dump MD EE info */
		CCCI_MEM_LOG_TAG(0, FSM, "Dump MD EX log\n");
		/*parse & dump md ee info*/
		if (ee_ctl->ops->dump_ee_info)
			ee_ctl->ops->dump_ee_info(ee_ctl,
				MDEE_DUMP_LEVEL_STAGE1, ee_case);

		/* Dump MD register*/
		md_dump_flag = DUMP_FLAG_REG | DUMP_FLAG_MD_WDT;

		if (ee_case == MD_EE_CASE_ONLY_SWINT)
			md_dump_flag |= (DUMP_FLAG_QUEUE_0
							| DUMP_FLAG_CCIF
							| DUMP_FLAG_CCIF_REG);
		ccci_md_dump_info(md_dump_flag, NULL, 0);

		/* check this first, as we overwrite share memory here */
		if (ee_case == MD_EE_CASE_NO_RESPONSE)
			ccci_md_dump_info(DUMP_FLAG_CCIF
			| DUMP_FLAG_CCIF_REG,
			mdss_dbg->base_ap_view_vir
			+ CCCI_EE_OFFSET_CCIF_SRAM,
			CCCI_EE_SIZE_CCIF_SRAM);

		/* Dump MD memory layout */
		CCCI_MEM_LOG_TAG(0, FSM, "Dump MD layout struct\n");
		ccci_util_mem_dump(CCCI_DUMP_MEM_DUMP, mem_layout,
			sizeof(struct ccci_mem_layout));
		/* Dump CCB memory */
		ccci_md_dump_info(
			DUMP_FLAG_SMEM_CCB_CTRL | DUMP_FLAG_SMEM_CCB_DATA,
			NULL, 0);

		CCCI_ERROR_LOG(0, FSM, "MD exception stage 1: end\n");
_dump_done:
		return;
	} else if (stage == 2) { /* got MD_EX_PASS or second timeout */
		unsigned long flags;
		unsigned int md_dump_flag = 0;
		struct ccci_smem_region *mdss_dbg
			= ccci_md_get_smem_by_user_id(SMEM_USER_RAW_MDSS_DBG);

#ifdef CUST_FT_EE_TRIGGER_REBOOT
		struct ccci_modem *md;
#endif
		CCCI_ERROR_LOG(0, FSM, "MD exception stage 2!\n");
		CCCI_MEM_LOG_TAG(0, FSM, "MD exception stage 2! ee=%x\n",
			ee_ctl->ee_info_flag);

		md_dump_flag = DUMP_FLAG_REG | DUMP_FLAG_MD_WDT;
		if (ee_ctl->ee_case == MD_EE_CASE_ONLY_SWINT)
			md_dump_flag |= (DUMP_FLAG_QUEUE_0
			| DUMP_FLAG_CCIF | DUMP_FLAG_CCIF_REG);
		ccci_md_dump_info(md_dump_flag, NULL, 0);
		/* check this first, as we overwrite share memory here */
		if (ee_ctl->ee_case == MD_EE_CASE_NO_RESPONSE)
			ccci_md_dump_info(
			DUMP_FLAG_CCIF | DUMP_FLAG_CCIF_REG,
			mdss_dbg->base_ap_view_vir
			+ CCCI_EE_OFFSET_CCIF_SRAM,
			CCCI_EE_SIZE_CCIF_SRAM);

		/*parse & dump md ee info*/
		if (ee_ctl->ops->dump_ee_info)
			ee_ctl->ops->dump_ee_info(ee_ctl,
				MDEE_DUMP_LEVEL_STAGE2, ee_ctl->ee_case);

		spin_lock_irqsave(&ee_ctl->ctrl_lock, flags);
		/* this flag should be the last action of
		 * a regular exception flow, clear flag
		 * for reset MD later
		 */
		ee_ctl->ee_info_flag = 0;
		spin_unlock_irqrestore(&ee_ctl->ctrl_lock, flags);

		CCCI_ERROR_LOG(0, FSM,
			"MD exception stage 2:end\n");
#ifdef CUST_FT_EE_TRIGGER_REBOOT
		md = ccci_get_modem();
		if (md == NULL) {
			CCCI_ERROR_LOG(0, FSM,
				"fail to get md struct-no any userful MD!!\n");
			return;
		}
		if (md->ccci_drv_trigger_upload) {
			CCCI_ERROR_LOG(0, FSM, "[md1] drv trigger panic\n");
			drv_tri_panic_by_lvl();
			return;
		}
		if (!ee_ctl->is_normal_ee_case) {
			ee_ctl->is_normal_ee_case = 0;
			CCCI_ERROR_LOG(0, FSM,
				"[md1] EE time out case, call panic\n");
			drv_tri_panic_by_lvl();
		}
#endif
	}
}

void fsm_md_wdt_handler(struct ccci_fsm_ee *ee_ctl)
{
	unsigned long flags;

	spin_lock_irqsave(&ee_ctl->ctrl_lock, flags);
	ee_ctl->ee_info_flag |= (MD_EE_FLOW_START | MD_EE_WDT_GET);
	spin_unlock_irqrestore(&ee_ctl->ctrl_lock, flags);
	fsm_md_exception_stage(ee_ctl, 1);
	msleep(MD_EX_PASS_TIMEOUT);
	fsm_md_exception_stage(ee_ctl, 2);
}

void fsm_md_no_response_handler(struct ccci_fsm_ee *ee_ctl)
{
	unsigned long flags;

	spin_lock_irqsave(&ee_ctl->ctrl_lock, flags);
	ee_ctl->ee_info_flag |= (MD_EE_FLOW_START | MD_EE_PENDING_TOO_LONG);
	spin_unlock_irqrestore(&ee_ctl->ctrl_lock, flags);
	fsm_md_exception_stage(ee_ctl, 1);
	msleep(MD_EX_PASS_TIMEOUT);
	fsm_md_exception_stage(ee_ctl, 2);
}

void fsm_ee_message_handler(struct ccci_fsm_ee *ee_ctl, struct sk_buff *skb)
{
	struct ccci_fsm_ctl *ctl
		= container_of(ee_ctl, struct ccci_fsm_ctl, ee_ctl);
	struct ccci_header *ccci_h = (struct ccci_header *)skb->data;
	unsigned long flags;
	enum MD_STATE md_state = ccci_fsm_get_md_state();

	if (md_state != EXCEPTION) {
		CCCI_ERROR_LOG(0, FSM,
			"receive invalid MD_EX %x when MD state is %d\n",
			ccci_h->reserved, md_state);
		return;
	}
	if (ccci_h->data[1] == MD_EX) {
		if (unlikely(ccci_h->reserved != MD_EX_CHK_ID)) {
			CCCI_ERROR_LOG(0, FSM,
				"receive invalid MD_EX %x\n",
				ccci_h->reserved);
		} else {
			spin_lock_irqsave(&ee_ctl->ctrl_lock, flags);
			ee_ctl->ee_info_flag
				|= (MD_EE_FLOW_START | MD_EE_MSG_GET);
			spin_unlock_irqrestore(&ee_ctl->ctrl_lock, flags);
			ccci_port_send_msg_to_md(
			CCCI_CONTROL_TX, MD_EX, MD_EX_CHK_ID, 1);
			fsm_append_event(ctl, CCCI_EVENT_MD_EX, NULL, 0);
		}
	} else if (ccci_h->data[1] == MD_EX_REC_OK) {
		if (unlikely(ccci_h->reserved != MD_EX_REC_OK_CHK_ID)) {
			CCCI_ERROR_LOG(0, FSM,
				"receive invalid MD_EX_REC_OK %x\n",
				ccci_h->reserved);
		} else {
			spin_lock_irqsave(&ee_ctl->ctrl_lock, flags);
			ee_ctl->ee_info_flag
				|= (MD_EE_FLOW_START | MD_EE_OK_MSG_GET);
			spin_unlock_irqrestore(&ee_ctl->ctrl_lock, flags);
			/* Keep exception info package from MD*/
			if (ee_ctl->ops->set_ee_pkg)
				ee_ctl->ops->set_ee_pkg(ee_ctl,
				skb_pull(skb, sizeof(struct ccci_header)),
				skb->len - sizeof(struct ccci_header));

			fsm_append_event(ctl,
				CCCI_EVENT_MD_EX_REC_OK, NULL, 0);
		}
	} else if (ccci_h->data[1] == MD_EX_PASS) {
		spin_lock_irqsave(&ee_ctl->ctrl_lock, flags);
		ee_ctl->ee_info_flag |= MD_EE_PASS_MSG_GET;
#ifdef CUST_FT_EE_TRIGGER_REBOOT
		/* dump share memory again to let MD check exception flow */
		ee_ctl->is_normal_ee_case = 1;
#endif
		spin_unlock_irqrestore(&ee_ctl->ctrl_lock, flags);
		fsm_append_event(ctl, CCCI_EVENT_MD_EX_PASS, NULL, 0);
	} else if (ccci_h->data[1] == CCCI_DRV_VER_ERROR) {
		CCCI_ERROR_LOG(0, FSM,
			"AP/MD driver version mis-match\n");
#if IS_ENABLED(CONFIG_MTK_AEE_FEATURE)
		aed_md_exception_api(NULL, 0, NULL,
			0, "AP/MD driver version mis-match\n",
			DB_OPT_DEFAULT);
#endif
	}
}

int fsm_check_ee_done(struct ccci_fsm_ee *ee_ctl, int timeout)
{
	int count = 0;
	bool is_ee_done = 0;
	int time_step = 200; /*ms*/
	int loop_max = timeout * 1000 / time_step;

	CCCI_BOOTUP_LOG(0, FSM, "checking EE status\n");
	while (ccci_fsm_get_md_state() == EXCEPTION) {
		if (ccci_port_get_critical_user(CRIT_USR_MDLOG)) {
			CCCI_DEBUG_LOG(0, FSM,
				"MD logger is running, waiting for EE dump done\n");
			is_ee_done = !(ee_ctl->ee_info_flag & MD_EE_FLOW_START)
				&& ee_ctl->mdlog_dump_done;
		} else
			is_ee_done = !(ee_ctl->ee_info_flag & MD_EE_FLOW_START);
		if (!is_ee_done) {
			msleep(time_step);
			count++;
		} else
			break;

		if (loop_max && (count > loop_max)) {
			CCCI_ERROR_LOG(0, FSM,
				"wait EE done timeout\n");
#ifdef DEBUG_FOR_CCB
			/* Dump CCB memory */
			ccci_port_dump_status();
			ccci_md_dump_info(DUMP_FLAG_CCIF |
				DUMP_FLAG_CCIF_REG | DUMP_FLAG_IRQ_STATUS |
				DUMP_FLAG_SMEM_CCB_CTRL |
				DUMP_FLAG_SMEM_CCB_DATA,
				NULL, 0);
#if IS_ENABLED(CONFIG_MTK_AEE_FEATURE)
			/*
			 * aee_kernel_warning("ccci_EE_timeout",
			 *	"MD EE debug: wait dump done timeout");
			 */
#endif
#endif
			return -1;
		}
	}
	CCCI_BOOTUP_LOG(0, FSM, "check EE done\n");
	return 0;
}

int fsm_ee_init(struct ccci_fsm_ee *ee_ctl)
{
	int ret = 0;
	struct ccci_modem *md = ccci_get_modem();

	spin_lock_init(&ee_ctl->ctrl_lock);
	if (md->hw_info->plat_val->md_gen  >= 6299)
		ret = mdee_dumper_v6_alloc(ee_ctl);
	else if (md->hw_info->plat_val->md_gen  >= 6297)
		ret = mdee_dumper_v5_alloc(ee_ctl);
	else if (md->hw_info->plat_val->md_gen  >= 6292)
		ret = mdee_dumper_v3_alloc(ee_ctl);
	else if (md->hw_info->plat_val->md_gen  == 6291)
		ret = mdee_dumper_v2_alloc(ee_ctl);
	return ret;
}


