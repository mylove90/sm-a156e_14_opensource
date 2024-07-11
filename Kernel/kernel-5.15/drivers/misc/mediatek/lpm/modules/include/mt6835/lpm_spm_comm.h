/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */


#ifndef __LPM_SPM_COMM_H__
#define __LPM_SPM_COMM_H__

#include <lpm_dbg_common_v1.h>
#include <lpm_pcm_def.h>

/* Determine for user name handle */
#define MT_LP_RQ_USER_NAME_LEN	(4)
#define MT_LP_RQ_USER_CHAR_U	(8)
#define MT_LP_RQ_USER_CHAR_MASK	(0xFF)

/* Determine for resource usage id */
#define MT_LP_RQ_ID_ALL_USAGE	(-1)

extern void __iomem *lpm_spm_base;

struct lpm_spm_wake_status {
	u32 r12;		/* SPM_BK_WAKE_EVENT */
	u32 r12_ext;		/* SPM_WAKEUP_EXT_STA */
	u32 md32pcm_wakeup_sta;	/* MD32CPM_WAKEUP_STA */
	u32 md32pcm_event_sta;	/* MD32PCM_EVENT_STA */
	u32 wake_misc;		/* SPM_BK_WAKE_MISC */
	u32 timer_out;		/* SPM_BK_PCM_TIMER */
	u32 r13;		/* PCM_REG13_DATA */
	u32 req_sta0;		/* SRC_REQ_STA_0 */
	u32 req_sta1;		/* SRC_REQ_STA_1 */
	u32 req_sta2;		/* SRC_REQ_STA_2 */
	u32 req_sta3;		/* SRC_REQ_STA_3 */
	u32 req_sta4;		/* SRC_REQ_STA_4 */
	u32 req_sta5;		/* SRC_REQ_STA_5 */
	u32 req_sta6;		/* SRC_REQ_STA_6 */
	u32 debug_flag;		/* PCM_WDT_LATCH_SPARE_0 */
	u32 debug_flag1;	/* PCM_WDT_LATCH_SPARE_1 */
	u32 isr;		/* SPM_IRQ_STA */
	u32 sw_flag0;		/* SPM_SW_FLAG_0 */
	u32 sw_flag1;		/* SPM_SW_FLAG_1 */
	u32 clk_settle;		/* SPM_CLK_SETTLE */
	u32 src_req;		/* SPM_SRC_REQ */
	u32 is_abort;
	u32 sw_rsv0;	/* SPM_SW_RSV_0 */
	u32 sw_rsv1;	/* SPM_SW_RSV_1 */
	u32 sw_rsv2;	/* SPM_SW_RSV_2 */
	u32 sw_rsv3;	/* SPM_SW_RSV_3 */
	u32 sw_rsv4;	/* SPM_SW_RSV_4 */
	u32 sw_rsv5;	/* SPM_SW_RSV_5 */
	u32 sw_rsv6;	/* SPM_SW_RSV_6 */
	u32 sw_rsv7;	/* SPM_SW_RSV_7 */
	u32 sw_rsv8;	/* SPM_SW_RSV_8 */
	u32 debug_spare2;	/* PCM_WDT_LATCH_SPARE_2 */
	u32 debug_spare3;	/* PCM_WDT_LATCH_SPARE_3 */
	u32 debug_spare4;	/* PCM_WDT_LATCH_SPARE_4 */
	u32 debug_spare5;	/* PCM_WDT_LATCH_SPARE_5 */
	u32 debug_spare6;	/* PCM_WDT_LATCH_SPARE_6 */
};

int lpm_spm_fs_init(void);
int lpm_spm_fs_deinit(void);

/*
 * Auto generated by DE, please DO NOT modify this file directly.
 * From Legacy sleep_def.h
 */
/* --- SPM Flag Define --- */
#define SPM_FLAG_DISABLE_INFRA_PDN               (1U << 0)
#define SPM_FLAG_DISABLE_DPM_PDN                 (1U << 1)
#define SPM_FLAG_DISABLE_MCUPM_PDN               (1U << 2)
#define SPM_FLAG_DISABLE_DPY_PDN                 (1U << 3)
#define SPM_FLAG_DISABLE_SYSRAM_SLEEP            (1U << 5)
#define SPM_FLAG_DISABLE_SSPM_SRAM_SLEEP         (1U << 6)
#define SPM_FLAG_DISABLE_BUS_CLK_OFF             (1U << 7)
#define SPM_FLAG_DISABLE_VCORE_DVS               (1U << 8)
#define SPM_FLAG_DISABLE_DDR_DFS                 (1U << 9)
#define SPM_FLAG_DISABLE_EMI_DFS                 (1U << 10)
#define SPM_FLAG_DISABLE_BUS_DFS                 (1U << 11)
#define SPM_FLAG_DISABLE_COMMON_SCENARIO         (1U << 12)
#define SPM_FLAG_ENABLE_LVTS_WORKAROUND          (1U << 13)
#define SPM_FLAG_DISABLE_CPU_PDN                 (1U << 14)
#define SPM_FLAG_DISABLE_ARMPLL_OFF              (1U << 15)
#define SPM_FLAG_KEEP_CSYSPWRACK_HIGH            (1U << 16)
#define SPM_FLAG_ENABLE_MET_DEBUG_USAGE          (1U << 17)
#define SPM_FLAG_ENABLE_SPM_DBG_WDT_DUMP         (1U << 18)
#define SPM_FLAG_RUN_COMMON_SCENARIO             (1U << 19)
#define SPM_FLAG_USE_SRCCLKENO2                  (1U << 20)
#define SPM_FLAG_DISABLE_DDRPHY_PDN              (1U << 23)
#define SPM_FLAG_DISABLE_DVFSQ                   (1U << 24)
#define SPM_FLAG_ENABLE_MD_MUMTAS                (1U << 25)
#define SPM_FLAG_VCORE_STATE                     (1U << 26)
#define SPM_FLAG_VTCXO_STATE                     (1U << 27)
#define SPM_FLAG_INFRA_STATE                     (1U << 28)
#define SPM_FLAG_APSRC_STATE                     (1U << 29)
#define SPM_FLAG_VRF18_STATE                     (1U << 30)
#define SPM_FLAG_DDREN_STATE                     (1U << 31)

/* --- SPM Flag1 Define --- */
#define SPM_FLAG1_DISABLE_AXI_BUS_TO_26M        (1U << 0)
#define SPM_FLAG1_DISABLE_SYSPLL_OFF            (1U << 1)
#define SPM_FLAG1_DISABLE_PWRAP_CLK_SWITCH      (1U << 2)
#define SPM_FLAG1_DISABLE_ULPOSC_OFF            (1U << 3)
#define SPM_FLAG1_FW_SET_ULPOSC_ON              (1U << 4)
#define SPM_FLAG1_DISABLE_EMI_CLK_TO_ULPOSC     (1U << 5)
#define SPM_FLAG1_DISABLE_NO_RESUME             (1U << 6)
#define SPM_FLAG1_RESERVED_BIT7                 (1U << 7)
#define SPM_FLAG1_RESERVED_BIT8                 (1U << 8)
#define SPM_FLAG1_RESERVED_BIT9                 (1U << 9)
#define SPM_FLAG1_DISABLE_SRCLKEN_LOW           (1U << 10)
#define SPM_FLAG1_DISABLE_SCP_CLK_SWITCH        (1U << 11)
#define SPM_FLAG1_DISABLE_TOP_26M_CK_OFF        (1U << 12)
#define SPM_FLAG1_DISABLE_PCM_26M_SWITCH        (1U << 13)
#define SPM_FLAG1_DISABLE_CKSQ_OFF              (1U << 14)
#define SPM_FLAG1_DISABLE_SUB_MEM_OFF           (1U << 15)
#define SPM_FLAG1_RESERVED_BIT16                (1U << 16)
#define SPM_FLAG1_RESERVED_BIT17                (1U << 17)
#define SPM_FLAG1_RESERVED_BIT18                (1U << 18)
#define SPM_FLAG1_RESERVED_BIT19                (1U << 19)
#define SPM_FLAG1_DISABLE_INFRA_SRAM_SLEEP      (1U << 20)
#define SPM_FLAG1_DISABLE_AXI_MEM_CLK_OFF       (1U << 21)
#define SPM_FLAG1_DISABLE_MCUPM_SRAM_SLEEP      (1U << 22)
#define SPM_FLAG1_RESERVED_BIT23                (1U << 23)
#define SPM_FLAG1_DISABLE_SCP_VREQ_MASK_CONTROL (1U << 24)
#define SPM_FLAG1_RESERVED_BIT25                (1U << 25)
#define SPM_FLAG1_RESERVED_BIT26                (1U << 26)
#define SPM_FLAG1_RESERVED_BIT27                (1U << 27)
#define SPM_FLAG1_RESERVED_BIT28                (1U << 28)
#define SPM_FLAG1_RESERVED_BIT29                (1U << 29)
#define SPM_FLAG1_RESERVED_BIT30                (1U << 30)
#define SPM_FLAG1_RESERVED_BIT31                (1U << 31)

/* --- SPM DEBUG Define --- */
#define	SPM_DBG_DEBUG_IDX_26M_WAKE              (1U << 0)
#define	SPM_DBG_DEBUG_IDX_26M_SLEEP             (1U << 1)
#define	SPM_DBG_DEBUG_IDX_INFRA_WAKE            (1U << 2)
#define	SPM_DBG_DEBUG_IDX_INFRA_SLEEP           (1U << 3)
#define	SPM_DBG_DEBUG_IDX_APSRC_WAKE            (1U << 4)
#define	SPM_DBG_DEBUG_IDX_APSRC_SLEEP           (1U << 5)
#define	SPM_DBG_DEBUG_IDX_VRF18_WAKE            (1U << 6)
#define	SPM_DBG_DEBUG_IDX_VRF18_SLEEP           (1U << 7)
#define	SPM_DBG_DEBUG_IDX_VCORE_WAKE            (1U << 8)
#define	SPM_DBG_DEBUG_IDX_VCORE_OFF             (1U << 9)
#define	SPM_DBG_DEBUG_IDX_DDREN_WAKE            (1U << 10)
#define	SPM_DBG_DEBUG_IDX_DDREN_SLEEP           (1U << 11)
#define	SPM_DBG_DEBUG_IDX_VCORE_LP_MODE         (1U << 12)
#define SPM_DBG_DEBUG_IDX_MCUPM_SRAM_SLP        (1U << 15)
#define SPM_DBG_DEBUG_IDX_SYSRAM_SLP            (1U << 16)
#define	SPM_DBG_DEBUG_IDX_SSPM_WFI              (1U << 17)
#define SPM_DBG_DEBUG_IDX_SSPM_SRAM_SLP         (1U << 18)
#define SPM_DBG_DEBUG_IDX_SSPM_ON               (1U << 19)
#define SPM_DBG_DEBUG_IDX_SYSRAM_ON             (1U << 20)
#define SPM_DBG_DEBUG_IDX_SPM_DVFS_NO_REQ       (1U << 21)
#define SPM_DBG_DEBUG_IDX_APSRC_SLEEP_ABORT     (1U << 23)
#define SPM_DBG_DEBUG_IDX_SPM_EMI_EBG_WA_DONE   (1U << 24)
#define SPM_DBG_DEBUG_IDX_SPM_NORMAL_WAKEUP     (1U << 28)
#define SPM_DBG_DEBUG_IDX_SPM_WAKEUP_BY_NONE    (1U << 29)
#define SPM_DBG_DEBUG_IDX_AXI_MEM_CLK_OFF       (1U << 30)
#define SPM_DBG_DEBUG_IDX_AXI_MEM_CLK_ON        (1U << 31)

/* --- SPM DEBUG1 Define --- */
#define SPM_DBG1_DEBUG_IDX_CURRENT_IS_LP                (1U << 0)
#define SPM_DBG1_DEBUG_IDX_VCORE_DVFS_START             (1U << 1)
#define SPM_DBG1_DEBUG_IDX_SYSPLL_OFF                   (1U << 2)
#define SPM_DBG1_DEBUG_IDX_SYSPLL_ON                    (1U << 3)
//#define SPM_DBG1_DEBUG_IDX_CURRENT_IS_VCORE_DFS       (1U << 4)
#define SPM_DBG1_DEBUG_IDX_INFRA_MTCMOS_OFF             (1U << 5)
#define SPM_DBG1_DEBUG_IDX_INFRA_MTCMOS_ON              (1U << 6)
#define SPM_DBG1_DEBUG_IDX_VTCXO_SLEEP_ABORT_0          (1U << 7)
#define SPM_DBG1_DEBUG_IDX_VTCXO_SLEEP_ABORT_1          (1U << 8)
#define SPM_DBG1_DEBUG_IDX_VCORE_SLEEP_ABORT_0          (1U << 9)
#define SPM_DBG1_DEBUG_IDX_VCORE_SLEEP_ABORT_1          (1U << 10)
#define SPM_DBG1_DEBUG_IDX_PWRAP_CLK_TO_ULPOSC          (1U << 11)
#define SPM_DBG1_DEBUG_IDX_PWRAP_CLK_TO_26M             (1U << 12)
#define SPM_DBG1_DEBUG_IDX_SCP_CLK_TO_32K               (1U << 13)
#define SPM_DBG1_DEBUG_IDX_SCP_CLK_TO_26M               (1U << 14)
#define SPM_DBG1_DEBUG_IDX_BUS_CLK_OFF                  (1U << 15)
#define SPM_DBG1_DEBUG_IDX_BUS_CLK_ON                   (1U << 16)
#define SPM_DBG1_DEBUG_IDX_SRCLKEN2_LOW                 (1U << 17)
#define SPM_DBG1_DEBUG_IDX_SRCLKEN2_HIGH                (1U << 18)
#define SPM_DBG1_DEBUG_IDX_ULPOSC_IS_OFF_BUT_SHOULD_ON  (1U << 20)
#define SPM_DBG1_DEBUG_IDX_PMIC_IRQ_ACK_LOW_ABORT       (1U << 21)
#define SPM_DBG1_DEBUG_IDX_PMIC_IRQ_ACK_HIGH_ABORT      (1U << 22)
#define SPM_DBG1_DEBUG_IDX_PWRAP_SLEEP_ACK_LOW_ABORT    (1U << 23)
#define SPM_DBG1_DEBUG_IDX_PWRAP_SLEEP_ACK_HIGH_ABORT   (1U << 24)
//#define SPM_DBG1_DEBUG_IDX_EMI_SLP_IDLE_ABORT         (1U << 25)
#define SPM_DBG1_DEBUG_IDX_SCP_SLP_ACK_LOW_ABORT        (1U << 26)
#define SPM_DBG1_DEBUG_IDX_SCP_SLP_ACK_HIGH_ABORT       (1U << 27)
#define SPM_DBG1_DEBUG_IDX_SPM_PMIF_CMD_RDY_ABORT       (1U << 28)
#define SPM_DBG1_DEBUG_IDX_MCUPM_RESORE                 (1U << 29)
#define SPM_DBG1_DEBUG_IDX_DISABLE_DVFSRC               (1U << 31)

#define is_infra_pdn(flags)		(!((flags) & SPM_FLAG_DISABLE_INFRA_PDN))

#endif
