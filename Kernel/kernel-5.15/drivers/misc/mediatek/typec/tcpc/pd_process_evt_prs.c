// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#include "inc/pd_dpm_core.h"
#include "inc/pd_process_evt.h"

#if CONFIG_USB_PD_PR_SWAP

#if CONFIG_USB_PD_PR_SWAP_ERROR_RECOVERY
#define PE_PRS_SNK_HARD_RESET	PE_ERROR_RECOVERY
#define PE_PRS_SRC_HARD_RESET	PE_ERROR_RECOVERY
#else
#define PE_PRS_SNK_HARD_RESET	PE_SNK_HARD_RESET
#define PE_PRS_SRC_HARD_RESET	PE_SRC_HARD_RESET
#endif	/* CONFIG_USB_PD_PR_SWAP_ERROR_RECOVERY */

/* PD Control MSG reactions */

DECL_PE_STATE_TRANSITION(PD_CTRL_MSG_GOOD_CRC) = {
	{ PE_PRS_SRC_SNK_ACCEPT_SWAP, PE_PRS_SRC_SNK_TRANSITION_TO_OFF },
	{ PE_PRS_SNK_SRC_ACCEPT_SWAP, PE_PRS_SNK_SRC_TRANSITION_TO_OFF },

	/* VBUS-ON & PS_RDY SENT */
	{ PE_PRS_SNK_SRC_SOURCE_ON, PE_SRC_STARTUP },
};
DECL_PE_STATE_REACTION(PD_CTRL_MSG_GOOD_CRC);

DECL_PE_STATE_TRANSITION(PD_CTRL_MSG_ACCEPT) = {
	{ PE_PRS_SRC_SNK_SEND_SWAP, PE_PRS_SRC_SNK_TRANSITION_TO_OFF },
	{ PE_PRS_SNK_SRC_SEND_SWAP, PE_PRS_SNK_SRC_TRANSITION_TO_OFF },
};
DECL_PE_STATE_REACTION(PD_CTRL_MSG_ACCEPT);

DECL_PE_STATE_TRANSITION(PD_CTRL_MSG_PS_RDY) = {
	{ PE_PRS_SRC_SNK_WAIT_SOURCE_ON, PE_SNK_STARTUP },
	{ PE_PRS_SNK_SRC_TRANSITION_TO_OFF, PE_PRS_SNK_SRC_ASSERT_RP },
};
DECL_PE_STATE_REACTION(PD_CTRL_MSG_PS_RDY);

/* DPM Event reactions */

DECL_PE_STATE_TRANSITION(PD_DPM_MSG_ACK) = {
	{ PE_PRS_SRC_SNK_EVALUATE_SWAP, PE_PRS_SRC_SNK_ACCEPT_SWAP },
	{ PE_PRS_SNK_SRC_EVALUATE_SWAP, PE_PRS_SNK_SRC_ACCEPT_SWAP },

	{ PE_PRS_SRC_SNK_ASSERT_RD, PE_PRS_SRC_SNK_WAIT_SOURCE_ON },
	{ PE_PRS_SNK_SRC_ASSERT_RP, PE_PRS_SNK_SRC_SOURCE_ON },
};
DECL_PE_STATE_REACTION(PD_DPM_MSG_ACK);

DECL_PE_STATE_TRANSITION(PD_DPM_MSG_NAK) = {
	{ PE_PRS_SRC_SNK_EVALUATE_SWAP, PE_PRS_SRC_SNK_REJECT_SWAP },
	{ PE_PRS_SNK_SRC_EVALUATE_SWAP, PE_PRS_SNK_SRC_REJECT_SWAP },
};
DECL_PE_STATE_REACTION(PD_DPM_MSG_NAK);

/* HW Event reactions */

DECL_PE_STATE_TRANSITION(PD_HW_MSG_TX_FAILED) = {
	{ PE_PRS_SRC_SNK_WAIT_SOURCE_ON, PE_PRS_SNK_HARD_RESET },
	{ PE_PRS_SNK_SRC_SOURCE_ON, PE_PRS_SRC_HARD_RESET },
};
DECL_PE_STATE_REACTION(PD_HW_MSG_TX_FAILED);

DECL_PE_STATE_TRANSITION(PD_HW_MSG_VBUS_SAFE0V) = {
	{ PE_PRS_SRC_SNK_TRANSITION_TO_OFF, PE_PRS_SRC_SNK_ASSERT_RD },
};
DECL_PE_STATE_REACTION(PD_HW_MSG_VBUS_SAFE0V);

/*
 * [BLOCK] Process PD Ctrl MSG
 */

static inline bool pd_process_ctrl_msg_good_crc(struct pd_port *pd_port)
{
	switch (pd_port->pe_state_curr) {
	case PE_PRS_SRC_SNK_WAIT_SOURCE_ON:
		pd_enable_pe_state_timer(pd_port, PD_TIMER_PS_SOURCE_ON);
		return false;

	default:
		return PE_MAKE_STATE_TRANSIT(PD_CTRL_MSG_GOOD_CRC);
	}
}

static inline bool pd_process_ctrl_msg(
	struct pd_port *pd_port, struct pd_event *pd_event)
{
	switch (pd_event->msg) {
	case PD_CTRL_GOOD_CRC:
		return pd_process_ctrl_msg_good_crc(pd_port);

	case PD_CTRL_ACCEPT:
		return PE_MAKE_STATE_TRANSIT(PD_CTRL_MSG_ACCEPT);

	case PD_CTRL_PS_RDY:
		return PE_MAKE_STATE_TRANSIT(PD_CTRL_MSG_PS_RDY);

	default:
		return false;
	}
}

/*
 * [BLOCK] Process DPM MSG
 */

static inline bool pd_process_dpm_msg(
	struct pd_port *pd_port, struct pd_event *pd_event)
{
	switch (pd_event->msg) {
	case PD_DPM_ACK:
		return PE_MAKE_STATE_TRANSIT(PD_DPM_MSG_ACK);

	case PD_DPM_NAK:
		return PE_MAKE_STATE_TRANSIT(PD_DPM_MSG_NAK);

	default:
		return false;
	}
}

/*
 * [BLOCK] Process HW MSG
 */

static inline bool pd_process_hw_msg(
	struct pd_port *pd_port, struct pd_event *pd_event)
{
	switch (pd_event->msg) {
	case PD_HW_VBUS_PRESENT:
		if (pd_port->pe_state_curr == PE_PRS_SNK_SRC_SOURCE_ON)
			pd_send_sop_ctrl_msg(pd_port, PD_CTRL_PS_RDY);

		return false;

	case PD_HW_TX_FAILED:
	case PD_HW_TX_DISCARD:
		return PE_MAKE_STATE_TRANSIT(PD_HW_MSG_TX_FAILED);

	case PD_HW_VBUS_SAFE0V:
		return PE_MAKE_STATE_TRANSIT(PD_HW_MSG_VBUS_SAFE0V);

	default:
		return false;
	}
}

/*
 * [BLOCK] Process Timer MSG
 */

static inline bool pd_process_timer_msg(
	struct pd_port *pd_port, struct pd_event *pd_event)
{
	switch (pd_event->msg) {
	case PD_TIMER_PS_SOURCE_ON:
		return PE_MAKE_STATE_TRANSIT_SINGLE(
			PE_PRS_SRC_SNK_WAIT_SOURCE_ON, PE_PRS_SNK_HARD_RESET);

	case PD_TIMER_PS_SOURCE_OFF:
		return PE_MAKE_STATE_TRANSIT_SINGLE(
			PE_PRS_SNK_SRC_TRANSITION_TO_OFF,
			PE_PRS_SNK_HARD_RESET);

	case PD_TIMER_SOURCE_TRANSITION:
		pd_notify_pe_execute_pr_swap(pd_port);
		pd_dpm_prs_enable_power_source(pd_port, false);
		pd_enable_pe_state_timer(pd_port, PD_TIMER_SOURCE_SWAP_STANDBY);
		return false;

	case PD_TIMER_SOURCE_SWAP_STANDBY:
		return PE_MAKE_STATE_TRANSIT_SINGLE(
			PE_PRS_SRC_SNK_TRANSITION_TO_OFF,
			PE_PRS_SRC_HARD_RESET);

	default:
		return false;
	}
}

/*
 * [BLOCK] Process Policy Engine's PRS Message
 */

bool pd_process_event_prs(struct pd_port *pd_port, struct pd_event *pd_event)
{
	switch (pd_event->event_type) {
	case PD_EVT_CTRL_MSG:
		return pd_process_ctrl_msg(pd_port, pd_event);

	case PD_EVT_DPM_MSG:
		return pd_process_dpm_msg(pd_port, pd_event);

	case PD_EVT_HW_MSG:
		return pd_process_hw_msg(pd_port, pd_event);

	case PD_EVT_TIMER_MSG:
		return pd_process_timer_msg(pd_port, pd_event);

	default:
		return false;
	}
}
#endif	/* CONFIG_USB_PD_PR_SWAP */

