/*
 * Copyright (c) 2012-2019 The Linux Foundation. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#if !defined(__SMEINSIDE_H)
#define __SMEINSIDE_H

/**
 * \file  sme_inside.h
 *
 * \brief prototype for SME structures and APIs used insside SME
 */

/*--------------------------------------------------------------------------
  Include Files
  ------------------------------------------------------------------------*/
#include "qdf_status.h"
#include "qdf_lock.h"
#include "qdf_trace.h"
#include "qdf_mem.h"
#include "qdf_types.h"
#include "sir_api.h"
#include "csr_internal.h"
#include "sme_qos_api.h"
#include "sme_qos_internal.h"

#include "sme_rrm_api.h"
#include "wlan_serialization_legacy_api.h"
ePhyChanBondState csr_convert_cb_ini_value_to_phy_cb_state(uint32_t cbIniValue);

/*--------------------------------------------------------------------------
  Type declarations
  ------------------------------------------------------------------------*/
/*
 * In case MAX num of STA are connected to SAP, switching off SAP causes
 * two SME cmd to be enqueued for each STA. Keeping SME total cmds as following
 * to make sure we have space for these cmds + some additional cmds.
 */
#define SME_TOTAL_COMMAND                (HAL_NUM_STA * 3)

typedef struct sGenericQosCmd {
	struct sme_qos_wmmtspecinfo tspecInfo;
	enum qca_wlan_ac_type ac;
	uint8_t tspec_mask;
} tGenericQosCmd;

/**
 * struct s_nss_update_cmd - Format of nss update request
 * @new_nss: new nss value
 * @session_id: Session ID
 * @set_hw_mode_cb: HDD nss update callback
 * @context: Adapter context
 * @next_action: Action to be taken after nss update
 * @reason: reason for nss update
 * @original_vdev_id: original request hwmode change vdev id
 */
struct s_nss_update_cmd {
	uint32_t new_nss;
	uint32_t session_id;
	void *nss_update_cb;
	void *context;
	uint8_t next_action;
	enum policy_mgr_conn_update_reason reason;
	uint32_t original_vdev_id;
};

typedef struct tagSmeCmd {
	tListElem Link;
	eSmeCommandType command;
	uint32_t cmd_id;
	uint32_t sessionId;
	union {
		struct roam_cmd roamCmd;
		struct wmstatus_changecmd wmStatusChangeCmd;
		tGenericQosCmd qosCmd;
		struct delstafor_sessionCmd delStaSessionCmd;
		struct policy_mgr_hw_mode set_hw_mode_cmd;
		struct s_nss_update_cmd nss_update_cmd;
		struct policy_mgr_dual_mac_config set_dual_mac_cmd;
		struct sir_antenna_mode_param set_antenna_mode_cmd;
	} u;
} tSmeCmd;

/*--------------------------------------------------------------------------
  Internal to SME
  ------------------------------------------------------------------------*/
/**
 * csr_get_cmd_type() - to convert sme command type to serialization cmd type
 * @sme_cmd: sme command pointer
 *
 * This API will convert SME command type to serialization command type which
 * new serialization module understands
 *
 * Return: serialization cmd type based on sme command type
 */
enum wlan_serialization_cmd_type csr_get_cmd_type(tSmeCmd *sme_cmd);
/**
 * csr_set_serialization_params_to_cmd() - take sme params and create new
 *						serialization command
 * @mac_ctx: pointer to mac context
 * @sme_cmd: sme command pointer
 * @cmd: serialization command pointer
 * @high_priority: if command is high priority
 *
 * Return: QDF_STATUS_SUCCESS or QDF_STATUS_E_FAILURE
 */
QDF_STATUS csr_set_serialization_params_to_cmd(struct mac_context *mac_ctx,
		tSmeCmd *sme_cmd, struct wlan_serialization_command *cmd,
		uint8_t high_priority);
tSmeCmd *sme_get_command_buffer(struct mac_context *mac);
void sme_release_command(struct mac_context *mac, tSmeCmd *pCmd);
bool qos_process_command(struct mac_context *mac, tSmeCmd *pCommand);
void qos_release_command(struct mac_context *mac, tSmeCmd *pCommand);
QDF_STATUS csr_roam_process_command(struct mac_context *mac, tSmeCmd *pCommand);

/**
 * csr_roam_wm_status_change_complete() - Remove WM status change command
 *                                        from SME active command list
 * @mac_ctx: global mac context
 * @session_id: session id
 *
 * This API removes WM status change command from SME active command list
 * if present.
 *
 * Return: void
 */
void csr_roam_wm_status_change_complete(struct mac_context *mac_ctx,
					uint8_t session_id);
void csr_roam_process_wm_status_change_command(struct mac_context *mac,
		tSmeCmd *pCommand);
/**
 * csr_process_del_sta_session_command() - Post WMA_DEL_STA_SELF_REQ to wma
 *
 * @mac_ctx: global mac context
 * @sme_command: received Delete Self station request command
 *
 * This API sends the WMA_DEL_STA_SELF_REQ msg to WMA.
 *
 * Return: QDF_STATUS_SUCCESS or QDF_STATUS_E_FAILURE
 */
QDF_STATUS csr_process_del_sta_session_command(struct mac_context *mac_ctx,
					       tSmeCmd *sme_command);
void csr_reinit_roam_cmd(struct mac_context *mac, tSmeCmd *pCommand);
void csr_reinit_wm_status_change_cmd(struct mac_context *mac, tSmeCmd *pCommand);
QDF_STATUS csr_roam_send_set_key_cmd(struct mac_context *mac_ctx,
		uint32_t session_id, struct setkey_cmd *set_key_cmd);
QDF_STATUS csr_is_valid_channel(struct mac_context *mac, uint8_t chnNum);

QDF_STATUS sme_acquire_global_lock(struct sme_context *sme);
QDF_STATUS sme_release_global_lock(struct sme_context *sme);

QDF_STATUS csr_process_add_sta_session_rsp(struct mac_context *mac, uint8_t *pMsg);
QDF_STATUS csr_process_del_sta_session_rsp(struct mac_context *mac, uint8_t *pMsg);

QDF_STATUS csr_flush_cfg_bg_scan_roam_channel_list(struct mac_context *mac,
		uint8_t sessionId);
QDF_STATUS csr_create_bg_scan_roam_channel_list(struct mac_context *mac,
		uint8_t sessionId, const uint8_t *pChannelList,
		const uint8_t numChannels);

#ifdef FEATURE_WLAN_ESE
QDF_STATUS csr_create_roam_scan_channel_list(struct mac_context *mac,
		uint8_t sessionId,
		uint8_t *pChannelList,
		uint8_t numChannels,
		const enum band_info band);
#endif

ePhyChanBondState csr_convert_cb_ini_value_to_phy_cb_state(uint32_t cbIniValue);
void csr_process_set_dual_mac_config(struct mac_context *mac, tSmeCmd *command);
void csr_process_set_antenna_mode(struct mac_context *mac, tSmeCmd *command);
void csr_process_set_hw_mode(struct mac_context *mac, tSmeCmd *command);
void csr_process_nss_update_req(struct mac_context *mac, tSmeCmd *command);
#endif /* #if !defined( __SMEINSIDE_H ) */
