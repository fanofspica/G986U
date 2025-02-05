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

#ifndef WLAN_POLICY_MGR_I_H
#define WLAN_POLICY_MGR_I_H

#include "wlan_policy_mgr_api.h"
#include "qdf_event.h"
#include "qdf_mc_timer.h"
#include "qdf_lock.h"
#include "qdf_defer.h"
#include "wlan_reg_services_api.h"

#define DBS_OPPORTUNISTIC_TIME   5

#define POLICY_MGR_SER_CMD_TIMEOUT 4000

#ifdef QCA_WIFI_3_0_EMU
#define CONNECTION_UPDATE_TIMEOUT (POLICY_MGR_SER_CMD_TIMEOUT + 3000)
#else
#define CONNECTION_UPDATE_TIMEOUT (POLICY_MGR_SER_CMD_TIMEOUT + 2000)
#endif

#define PM_24_GHZ_CHANNEL_6   (6)
#define PM_5_GHZ_CHANNEL_36   (36)
#define CHANNEL_SWITCH_COMPLETE_TIMEOUT   (2000)

/**
 * Policy Mgr hardware mode list bit-mask definitions.
 * Bits 4:0, 31:29 are unused.
 *
 * The below definitions are added corresponding to WMI DBS HW mode
 * list to make it independent of firmware changes for WMI definitions.
 * Currently these definitions have dependency with BIT positions of
 * the existing WMI macros. Thus, if the BIT positions are changed for
 * WMI macros, then these macros' BIT definitions are also need to be
 * changed.
 */
#define POLICY_MGR_HW_MODE_MAC0_TX_STREAMS_BITPOS  (28)
#define POLICY_MGR_HW_MODE_MAC0_RX_STREAMS_BITPOS  (24)
#define POLICY_MGR_HW_MODE_MAC1_TX_STREAMS_BITPOS  (20)
#define POLICY_MGR_HW_MODE_MAC1_RX_STREAMS_BITPOS  (16)
#define POLICY_MGR_HW_MODE_MAC0_BANDWIDTH_BITPOS   (12)
#define POLICY_MGR_HW_MODE_MAC1_BANDWIDTH_BITPOS   (8)
#define POLICY_MGR_HW_MODE_DBS_MODE_BITPOS         (7)
#define POLICY_MGR_HW_MODE_AGILE_DFS_MODE_BITPOS   (6)
#define POLICY_MGR_HW_MODE_SBS_MODE_BITPOS         (5)
#define POLICY_MGR_HW_MODE_MAC0_BAND_BITPOS        (3)
#define POLICY_MGR_HW_MODE_ID_BITPOS               (0)

#define POLICY_MGR_HW_MODE_MAC0_TX_STREAMS_MASK    \
	(0xf << POLICY_MGR_HW_MODE_MAC0_TX_STREAMS_BITPOS)
#define POLICY_MGR_HW_MODE_MAC0_RX_STREAMS_MASK    \
	(0xf << POLICY_MGR_HW_MODE_MAC0_RX_STREAMS_BITPOS)
#define POLICY_MGR_HW_MODE_MAC1_TX_STREAMS_MASK    \
	(0xf << POLICY_MGR_HW_MODE_MAC1_TX_STREAMS_BITPOS)
#define POLICY_MGR_HW_MODE_MAC1_RX_STREAMS_MASK    \
	(0xf << POLICY_MGR_HW_MODE_MAC1_RX_STREAMS_BITPOS)
#define POLICY_MGR_HW_MODE_MAC0_BANDWIDTH_MASK     \
	(0xf << POLICY_MGR_HW_MODE_MAC0_BANDWIDTH_BITPOS)
#define POLICY_MGR_HW_MODE_MAC1_BANDWIDTH_MASK     \
	(0xf << POLICY_MGR_HW_MODE_MAC1_BANDWIDTH_BITPOS)
#define POLICY_MGR_HW_MODE_DBS_MODE_MASK           \
	(0x1 << POLICY_MGR_HW_MODE_DBS_MODE_BITPOS)
#define POLICY_MGR_HW_MODE_AGILE_DFS_MODE_MASK     \
	(0x1 << POLICY_MGR_HW_MODE_AGILE_DFS_MODE_BITPOS)
#define POLICY_MGR_HW_MODE_SBS_MODE_MASK           \
	(0x1 << POLICY_MGR_HW_MODE_SBS_MODE_BITPOS)
#define POLICY_MGR_HW_MODE_MAC0_BAND_MASK           \
			(0x3 << POLICY_MGR_HW_MODE_MAC0_BAND_BITPOS)
#define POLICY_MGR_HW_MODE_ID_MASK           \
			(0x7 << POLICY_MGR_HW_MODE_ID_BITPOS)

#define POLICY_MGR_HW_MODE_MAC0_TX_STREAMS_SET(hw_mode, value) \
	WMI_SET_BITS(hw_mode, POLICY_MGR_HW_MODE_MAC0_TX_STREAMS_BITPOS,\
	4, value)
#define POLICY_MGR_HW_MODE_MAC0_RX_STREAMS_SET(hw_mode, value) \
	WMI_SET_BITS(hw_mode, POLICY_MGR_HW_MODE_MAC0_RX_STREAMS_BITPOS,\
	4, value)
#define POLICY_MGR_HW_MODE_MAC1_TX_STREAMS_SET(hw_mode, value) \
	WMI_SET_BITS(hw_mode, POLICY_MGR_HW_MODE_MAC1_TX_STREAMS_BITPOS,\
	4, value)
#define POLICY_MGR_HW_MODE_MAC1_RX_STREAMS_SET(hw_mode, value) \
	WMI_SET_BITS(hw_mode, POLICY_MGR_HW_MODE_MAC1_RX_STREAMS_BITPOS,\
	4, value)
#define POLICY_MGR_HW_MODE_MAC0_BANDWIDTH_SET(hw_mode, value)  \
	WMI_SET_BITS(hw_mode, POLICY_MGR_HW_MODE_MAC0_BANDWIDTH_BITPOS,\
	4, value)
#define POLICY_MGR_HW_MODE_MAC1_BANDWIDTH_SET(hw_mode, value)  \
	WMI_SET_BITS(hw_mode, POLICY_MGR_HW_MODE_MAC1_BANDWIDTH_BITPOS,\
	4, value)
#define POLICY_MGR_HW_MODE_DBS_MODE_SET(hw_mode, value)        \
	WMI_SET_BITS(hw_mode, POLICY_MGR_HW_MODE_DBS_MODE_BITPOS,\
	1, value)
#define POLICY_MGR_HW_MODE_AGILE_DFS_SET(hw_mode, value)       \
	WMI_SET_BITS(hw_mode, POLICY_MGR_HW_MODE_AGILE_DFS_MODE_BITPOS,\
	1, value)
#define POLICY_MGR_HW_MODE_SBS_MODE_SET(hw_mode, value)        \
	WMI_SET_BITS(hw_mode, POLICY_MGR_HW_MODE_SBS_MODE_BITPOS,\
	1, value)
#define POLICY_MGR_HW_MODE_MAC0_BAND_SET(hw_mode, value)        \
	WMI_SET_BITS(hw_mode, POLICY_MGR_HW_MODE_MAC0_BAND_BITPOS,\
	2, value)
#define POLICY_MGR_HW_MODE_ID_SET(hw_mode, value)        \
	WMI_SET_BITS(hw_mode, POLICY_MGR_HW_MODE_ID_BITPOS,\
	3, value)

#define POLICY_MGR_HW_MODE_MAC0_TX_STREAMS_GET(hw_mode)                \
		(((hw_mode) & POLICY_MGR_HW_MODE_MAC0_TX_STREAMS_MASK) >>    \
		POLICY_MGR_HW_MODE_MAC0_TX_STREAMS_BITPOS)
#define POLICY_MGR_HW_MODE_MAC0_RX_STREAMS_GET(hw_mode)                \
		(((hw_mode) & POLICY_MGR_HW_MODE_MAC0_RX_STREAMS_MASK) >>    \
		POLICY_MGR_HW_MODE_MAC0_RX_STREAMS_BITPOS)
#define POLICY_MGR_HW_MODE_MAC1_TX_STREAMS_GET(hw_mode)                \
		(((hw_mode) & POLICY_MGR_HW_MODE_MAC1_TX_STREAMS_MASK) >>    \
		POLICY_MGR_HW_MODE_MAC1_TX_STREAMS_BITPOS)
#define POLICY_MGR_HW_MODE_MAC1_RX_STREAMS_GET(hw_mode)                \
		(((hw_mode) & POLICY_MGR_HW_MODE_MAC1_RX_STREAMS_MASK) >>    \
		POLICY_MGR_HW_MODE_MAC1_RX_STREAMS_BITPOS)
#define POLICY_MGR_HW_MODE_MAC0_BANDWIDTH_GET(hw_mode)                 \
		(((hw_mode) & POLICY_MGR_HW_MODE_MAC0_BANDWIDTH_MASK) >>     \
		POLICY_MGR_HW_MODE_MAC0_BANDWIDTH_BITPOS)
#define POLICY_MGR_HW_MODE_MAC1_BANDWIDTH_GET(hw_mode)                 \
		(((hw_mode) & POLICY_MGR_HW_MODE_MAC1_BANDWIDTH_MASK) >>     \
		POLICY_MGR_HW_MODE_MAC1_BANDWIDTH_BITPOS)
#define POLICY_MGR_HW_MODE_DBS_MODE_GET(hw_mode)                       \
		(((hw_mode) & POLICY_MGR_HW_MODE_DBS_MODE_MASK) >>           \
		POLICY_MGR_HW_MODE_DBS_MODE_BITPOS)
#define POLICY_MGR_HW_MODE_AGILE_DFS_GET(hw_mode)                      \
		(((hw_mode) & POLICY_MGR_HW_MODE_AGILE_DFS_MODE_MASK) >>     \
		POLICY_MGR_HW_MODE_AGILE_DFS_MODE_BITPOS)
#define POLICY_MGR_HW_MODE_SBS_MODE_GET(hw_mode)                       \
		(((hw_mode) & POLICY_MGR_HW_MODE_SBS_MODE_MASK) >>           \
		POLICY_MGR_HW_MODE_SBS_MODE_BITPOS)
#define POLICY_MGR_HW_MODE_MAC0_BAND_GET(hw_mode)                       \
		(((hw_mode) & POLICY_MGR_HW_MODE_MAC0_BAND_MASK) >> \
		POLICY_MGR_HW_MODE_MAC0_BAND_BITPOS)
#define POLICY_MGR_HW_MODE_ID_GET(hw_mode)                       \
		(((hw_mode) & POLICY_MGR_HW_MODE_ID_MASK) >> \
		POLICY_MGR_HW_MODE_ID_BITPOS)

#define POLICY_MGR_DEFAULT_HW_MODE_INDEX 0xFFFF

#define policy_mgr_alert(params...) \
	QDF_TRACE_FATAL(QDF_MODULE_ID_POLICY_MGR, params)
#define policy_mgr_err(params...) \
	QDF_TRACE_ERROR(QDF_MODULE_ID_POLICY_MGR, params)
#define policy_mgr_warn(params...) \
	QDF_TRACE_WARN(QDF_MODULE_ID_POLICY_MGR, params)
#define policy_mgr_notice(params...) \
	QDF_TRACE_INFO(QDF_MODULE_ID_POLICY_MGR, params)
#define policy_mgr_info(params...) \
	QDF_TRACE_INFO(QDF_MODULE_ID_POLICY_MGR, params)
#define policy_mgr_debug(params...) \
	QDF_TRACE_DEBUG(QDF_MODULE_ID_POLICY_MGR, params)

#define policymgr_nofl_alert(params...) \
	QDF_TRACE_FATAL_NO_FL(QDF_MODULE_ID_POLICY_MGR, params)
#define policymgr_nofl_err(params...) \
	QDF_TRACE_ERROR_NO_FL(QDF_MODULE_ID_POLICY_MGR, params)
#define policymgr_nofl_warn(params...) \
	QDF_TRACE_WARN_NO_FL(QDF_MODULE_ID_POLICY_MGR, params)
#define policymgr_nofl_info(params...) \
	QDF_TRACE_INFO_NO_FL(QDF_MODULE_ID_POLICY_MGR, params)
#define policymgr_nofl_debug(params...) \
	QDF_TRACE_DEBUG_NO_FL(QDF_MODULE_ID_POLICY_MGR, params)

#define PM_CONC_CONNECTION_LIST_VALID_INDEX(index) \
		((MAX_NUMBER_OF_CONC_CONNECTIONS > index) && \
			(pm_conc_connection_list[index].in_use))

extern struct policy_mgr_conc_connection_info
	pm_conc_connection_list[MAX_NUMBER_OF_CONC_CONNECTIONS];

extern const enum policy_mgr_pcl_type
	first_connection_pcl_table[PM_MAX_NUM_OF_MODE]
			[PM_MAX_CONC_PRIORITY_MODE];
extern pm_dbs_pcl_second_connection_table_type
		*second_connection_pcl_dbs_table;
extern pm_dbs_pcl_third_connection_table_type
		*third_connection_pcl_dbs_table;
extern policy_mgr_next_action_two_connection_table_type
		*next_action_two_connection_table;
extern policy_mgr_next_action_three_connection_table_type
		*next_action_three_connection_table;
extern policy_mgr_next_action_two_connection_table_type
		*next_action_two_connection_2x2_2g_1x1_5g_table;
extern policy_mgr_next_action_three_connection_table_type
		*next_action_three_connection_2x2_2g_1x1_5g_table;

extern enum policy_mgr_conc_next_action
	(*policy_mgr_get_current_pref_hw_mode_ptr)
	(struct wlan_objmgr_psoc *psoc);

/**
 * struct sta_ap_intf_check_work_ctx - sta_ap_intf_check_work
 * related info
 * @psoc: pointer to PSOC object information
 */
struct sta_ap_intf_check_work_ctx {
	struct wlan_objmgr_psoc *psoc;
};

/**
 * struct policy_mgr_cfg - all the policy manager owned configs
 * @mcc_to_scc_switch: switch to indicate MCC to SCC config
 * @sys_pref: system's preference while selecting PCLs
 * @max_conc_cxns: Max allowed concurrenct active connections
 * @conc_rule1: concurrency rule1
 * @conc_rule2: concurrency rule2
 * @allow_mcc_go_diff_bi: Allow GO and STA diff beacon interval in MCC
 * @enable_overlap_chnl: Enable overlap channels for SAP's channel selection
 * @dual_mac_feature: To enable/disable dual mac features
 * @is_force_1x1_enable: Is 1x1 forced for connection
 * @sta_sap_scc_on_dfs_chnl: STA-SAP SCC on DFS channel
 * @sta_sap_scc_on_lte_coex_chnl: STA-SAP SCC on LTE Co-ex channel
 * @nan_sap_scc_on_lte_coex_chnl: NAN-SAP SCC on LTE Co-ex channel
 * @sap_mandatory_chnl_enable: To enable/disable SAP mandatory channels
 * @mark_indoor_chnl_disable: Mark indoor channel as disable or enable
 * @dbs_selection_plcy: DBS selection policy for concurrency
 * @vdev_priority_list: Priority list for various vdevs
 * @chnl_select_plcy: Channel selection policy
 * @enable_mcc_adaptive_sch: Enable/Disable MCC adaptive scheduler
 * @enable_sta_cxn_5g_band: Enable/Disable STA connection in 5G band
 */
struct policy_mgr_cfg {
	uint8_t mcc_to_scc_switch;
	uint8_t sys_pref;
	uint8_t max_conc_cxns;
	uint8_t conc_rule1;
	uint8_t conc_rule2;
	uint8_t enable_mcc_adaptive_sch;
	uint8_t allow_mcc_go_diff_bi;
	uint8_t enable_overlap_chnl;
	uint8_t dual_mac_feature;
	uint8_t is_force_1x1_enable;
	uint8_t sta_sap_scc_on_dfs_chnl;
	uint8_t sta_sap_scc_on_lte_coex_chnl;
	uint8_t nan_sap_scc_on_lte_coex_chnl;
	uint8_t sap_mandatory_chnl_enable;
	uint8_t mark_indoor_chnl_disable;
	uint8_t enable_sta_cxn_5g_band;
	uint32_t dbs_selection_plcy;
	uint32_t vdev_priority_list;
	uint32_t chnl_select_plcy;
};

/**
 * struct policy_mgr_psoc_priv_obj - Policy manager private data
 * @psoc: pointer to PSOC object information
 * @pdev: pointer to PDEV object information
 * @connection_update_done_evt: qdf event to synchronize
 *                            connection activities
 * @qdf_conc_list_lock: To protect connection table
 * @dbs_opportunistic_timer: Timer to drop down to Single Mac
 *                         Mode opportunistically
 * @sap_restart_chan_switch_cb: Callback for channel switch
 *                            notification for SAP
 * @sme_cbacks: callbacks to be registered by SME for
 *            interaction with Policy Manager
 * @wma_cbacks: callbacks to be registered by SME for
 * interaction with Policy Manager
 * @tdls_cbacks: callbacks to be registered by SME for
 * interaction with Policy Manager
 * @cdp_cbacks: callbacks to be registered by SME for
 * interaction with Policy Manager
 * @sap_mandatory_channels: The user preferred master list on
 *                        which SAP can be brought up. This
 *                        mandatory channel list would be as per
 *                        OEMs preference & conforming to the
 *                        regulatory/other considerations
 * @sap_mandatory_channels_len: Length of the SAP mandatory
 *                            channel list
 * @do_hw_mode_change: Flag to check if HW mode change is needed
 *                   after vdev is up. Especially used after
 *                   channel switch related vdev restart
 * @concurrency_mode: active concurrency combination
 * @no_of_open_sessions: Number of active vdevs
 * @no_of_active_sessions: Number of active connections
 * @sta_ap_intf_check_work: delayed sap restart work
 * @num_dbs_hw_modes: Number of different HW modes supported
 * @hw_mode: List of HW modes supported
 * @old_hw_mode_index: Old HW mode from hw_mode table
 * @new_hw_mode_index: New HW mode from hw_mode table
 * @dual_mac_cfg: DBS configuration currenctly used by FW for
 *              scan & connections
 * @hw_mode_change_in_progress: This is to track if HW mode
 *                            change is in progress
 * @enable_mcc_adaptive_scheduler: Enable MCC adaptive scheduler
 *      value from INI
 * @unsafe_channel_list: LTE coex channel avoidance list
 * @unsafe_channel_count: LTE coex channel avoidance list count
 * @sta_ap_intf_check_work_info: Info related to sta_ap_intf_check_work
 * @nan_sap_conc_work: Info related to nan sap conc work
 * @opportunistic_update_done_evt: qdf event to synchronize host
 *                               & FW HW mode
 */
struct policy_mgr_psoc_priv_obj {
	struct wlan_objmgr_psoc *psoc;
	struct wlan_objmgr_pdev *pdev;
	qdf_event_t connection_update_done_evt;
	qdf_mutex_t qdf_conc_list_lock;
	qdf_mc_timer_t dbs_opportunistic_timer;
	struct policy_mgr_hdd_cbacks hdd_cbacks;
	struct policy_mgr_sme_cbacks sme_cbacks;
	struct policy_mgr_wma_cbacks wma_cbacks;
	struct policy_mgr_tdls_cbacks tdls_cbacks;
	struct policy_mgr_cdp_cbacks cdp_cbacks;
	struct policy_mgr_dp_cbacks dp_cbacks;
	uint8_t sap_mandatory_channels[QDF_MAX_NUM_CHAN];
	uint32_t sap_mandatory_channels_len;
	bool do_hw_mode_change;
	uint32_t concurrency_mode;
	uint8_t no_of_open_sessions[QDF_MAX_NO_OF_MODE];
	uint8_t no_of_active_sessions[QDF_MAX_NO_OF_MODE];
	qdf_work_t sta_ap_intf_check_work;
	qdf_work_t nan_sap_conc_work;
	uint32_t num_dbs_hw_modes;
	struct dbs_hw_mode_info hw_mode;
	uint32_t old_hw_mode_index;
	uint32_t new_hw_mode_index;
	struct dual_mac_config dual_mac_cfg;
	uint32_t hw_mode_change_in_progress;
	struct policy_mgr_user_cfg user_cfg;
	uint16_t unsafe_channel_list[QDF_MAX_NUM_CHAN];
	uint16_t unsafe_channel_count;
	struct sta_ap_intf_check_work_ctx *sta_ap_intf_check_work_info;
	uint8_t cur_conc_system_pref;
	qdf_event_t opportunistic_update_done_evt;
	qdf_event_t channel_switch_complete_evt;
	send_mode_change_event_cb mode_change_cb;
	uint32_t user_config_sap_channel;
	struct policy_mgr_cfg cfg;
};

/**
 * struct policy_mgr_mac_ss_bw_info - hw_mode_list PHY/MAC params for each MAC
 * @mac_tx_stream: Max TX stream number supported on MAC
 * @mac_rx_stream: Max RX stream number supported on MAC
 * @mac_bw: Max bandwidth(wmi_channel_width enum type)
 * @mac_band_cap: supported Band bit map(WLAN_2G_CAPABILITY = 0x1,
 *                            WLAN_5G_CAPABILITY = 0x2)
 */
struct policy_mgr_mac_ss_bw_info {
	uint32_t mac_tx_stream;
	uint32_t mac_rx_stream;
	uint32_t mac_bw;
	uint32_t mac_band_cap;
};

struct policy_mgr_psoc_priv_obj *policy_mgr_get_context(
		struct wlan_objmgr_psoc *psoc);
QDF_STATUS policy_mgr_get_updated_scan_config(
		struct wlan_objmgr_psoc *psoc,
		uint32_t *scan_config,
		bool dbs_scan,
		bool dbs_plus_agile_scan,
		bool single_mac_scan_with_dfs);
QDF_STATUS policy_mgr_get_updated_fw_mode_config(
		struct wlan_objmgr_psoc *psoc,
		uint32_t *fw_mode_config,
		bool dbs,
		bool agile_dfs);
bool policy_mgr_is_dual_mac_disabled_in_ini(
		struct wlan_objmgr_psoc *psoc);

/**
 * policy_mgr_get_mcc_to_scc_switch_mode() - MCC to SCC
 * switch mode value in the user config
 * @psoc: PSOC object information
 *
 * MCC to SCC switch mode value in user config
 *
 * Return: MCC to SCC switch mode value
 */
uint32_t policy_mgr_get_mcc_to_scc_switch_mode(
	struct wlan_objmgr_psoc *psoc);
bool policy_mgr_get_dbs_config(struct wlan_objmgr_psoc *psoc);
bool policy_mgr_get_agile_dfs_config(struct wlan_objmgr_psoc *psoc);
bool policy_mgr_get_dbs_scan_config(struct wlan_objmgr_psoc *psoc);
void policy_mgr_get_tx_rx_ss_from_config(enum hw_mode_ss_config mac_ss,
		uint32_t *tx_ss, uint32_t *rx_ss);
int8_t policy_mgr_get_matching_hw_mode_index(
		struct wlan_objmgr_psoc *psoc,
		uint32_t mac0_tx_ss, uint32_t mac0_rx_ss,
		enum hw_mode_bandwidth mac0_bw,
		uint32_t mac1_tx_ss, uint32_t mac1_rx_ss,
		enum hw_mode_bandwidth mac1_bw,
		enum hw_mode_mac_band_cap mac0_band_cap,
		enum hw_mode_dbs_capab dbs,
		enum hw_mode_agile_dfs_capab dfs,
		enum hw_mode_sbs_capab sbs);
int8_t policy_mgr_get_hw_mode_idx_from_dbs_hw_list(
		struct wlan_objmgr_psoc *psoc,
		enum hw_mode_ss_config mac0_ss,
		enum hw_mode_bandwidth mac0_bw,
		enum hw_mode_ss_config mac1_ss,
		enum hw_mode_bandwidth mac1_bw,
		enum hw_mode_mac_band_cap mac0_band_cap,
		enum hw_mode_dbs_capab dbs,
		enum hw_mode_agile_dfs_capab dfs,
		enum hw_mode_sbs_capab sbs);
QDF_STATUS policy_mgr_get_old_and_new_hw_index(
		struct wlan_objmgr_psoc *psoc,
		uint32_t *old_hw_mode_index,
		uint32_t *new_hw_mode_index);
void policy_mgr_update_conc_list(struct wlan_objmgr_psoc *psoc,
		uint32_t conn_index,
		enum policy_mgr_con_mode mode,
		uint8_t chan,
		enum hw_mode_bandwidth bw,
		uint8_t mac,
		enum policy_mgr_chain_mode chain_mask,
		uint32_t original_nss,
		uint32_t vdev_id,
		bool in_use,
		bool update_conn);
void policy_mgr_store_and_del_conn_info(struct wlan_objmgr_psoc *psoc,
				enum policy_mgr_con_mode mode,
				bool all_matching_cxn_to_del,
				struct policy_mgr_conc_connection_info *info,
				uint8_t *num_cxn_del);

/**
 * policy_mgr_store_and_del_conn_info_by_vdev_id() - Store and del a
 * connection info by vdev id
 * @psoc: PSOC object information
 * @vdev_id: vdev id whose entry has to be deleted
 * @info: struture array pointer where the connection info will be saved
 * @num_cxn_del: number of connection which are going to be deleted
 *
 * Saves the connection info corresponding to the provided mode
 * and deleted that corresponding entry based on vdev from the
 * connection info structure
 *
 * Return: None
 */
void policy_mgr_store_and_del_conn_info_by_vdev_id(
			struct wlan_objmgr_psoc *psoc,
			uint32_t vdev_id,
			struct policy_mgr_conc_connection_info *info,
			uint8_t *num_cxn_del);

void policy_mgr_restore_deleted_conn_info(struct wlan_objmgr_psoc *psoc,
				struct policy_mgr_conc_connection_info *info,
				uint8_t num_cxn_del);
void policy_mgr_update_hw_mode_conn_info(struct wlan_objmgr_psoc *psoc,
				uint32_t num_vdev_mac_entries,
				struct policy_mgr_vdev_mac_map *vdev_mac_map,
				struct policy_mgr_hw_mode_params hw_mode);
void policy_mgr_pdev_set_hw_mode_cb(uint32_t status,
				uint32_t cfgd_hw_mode_index,
				uint32_t num_vdev_mac_entries,
				struct policy_mgr_vdev_mac_map *vdev_mac_map,
				uint8_t next_action,
				enum policy_mgr_conn_update_reason reason,
				uint32_t session_id, void *context);
void policy_mgr_dump_current_concurrency(struct wlan_objmgr_psoc *psoc);

/**
 * policy_mgr_pdev_get_pcl() - GET PCL channel list
 * @psoc: PSOC object information
 * @mode: Adapter mode
 * @pcl: the pointer of pcl list
 *
 * Fetches the PCL.
 *
 * Return: QDF_STATUS
 */
QDF_STATUS policy_mgr_pdev_get_pcl(struct wlan_objmgr_psoc *psoc,
				   enum QDF_OPMODE mode,
				   struct policy_mgr_pcl_list *pcl);
void pm_dbs_opportunistic_timer_handler(void *data);
enum policy_mgr_con_mode policy_mgr_get_mode(uint8_t type,
		uint8_t subtype);
enum hw_mode_bandwidth policy_mgr_get_bw(enum phy_ch_width chan_width);

/**
 * policy_mgr_get_bw() - Convert hw_mode_bandwidth to phy_ch_width
 * @bw: Hardware mode band width used by WMI
 *
 * Return: phy_ch_width
 */
enum phy_ch_width policy_mgr_get_ch_width(enum hw_mode_bandwidth bw);

QDF_STATUS policy_mgr_get_channel_list(struct wlan_objmgr_psoc *psoc,
			enum policy_mgr_pcl_type pcl,
			uint8_t *pcl_channels, uint32_t *len,
			enum policy_mgr_con_mode mode,
			uint8_t *pcl_weights, uint32_t weight_len);
bool policy_mgr_allow_new_home_channel(struct wlan_objmgr_psoc *psoc,
			uint8_t channel, uint32_t num_connections);
bool policy_mgr_is_5g_channel_allowed(struct wlan_objmgr_psoc *psoc,
				uint8_t channel, uint32_t *list,
				enum policy_mgr_con_mode mode);
QDF_STATUS policy_mgr_complete_action(struct wlan_objmgr_psoc *psoc,
				uint8_t  new_nss, uint8_t next_action,
				enum policy_mgr_conn_update_reason reason,
				uint32_t session_id);
enum policy_mgr_con_mode policy_mgr_get_mode_by_vdev_id(
		struct wlan_objmgr_psoc *psoc,
		uint8_t vdev_id);
QDF_STATUS policy_mgr_init_connection_update(
		struct policy_mgr_psoc_priv_obj *pm_ctx);
enum policy_mgr_conc_next_action
		policy_mgr_get_current_pref_hw_mode_dbs_2x2(
		struct wlan_objmgr_psoc *psoc);
enum policy_mgr_conc_next_action
		policy_mgr_get_current_pref_hw_mode_dbs_1x1(
		struct wlan_objmgr_psoc *psoc);

/**
 * policy_mgr_get_current_pref_hw_mode_dual_dbs() - Get the
 * current preferred hw mode
 *
 * Get the preferred hw mode based on the current connection combinations
 *
 * Return: No change (PM_NOP), (PM_SINGLE_MAC_UPGRADE),
 *         DBS (PM_DBS1_DOWNGRADE or PM_DBS2_DOWNGRADE)
 */
enum policy_mgr_conc_next_action
		policy_mgr_get_current_pref_hw_mode_dual_dbs(
		struct wlan_objmgr_psoc *psoc);

QDF_STATUS policy_mgr_reset_sap_mandatory_channels(
		struct policy_mgr_psoc_priv_obj *pm_ctx);

/**
 * policy_mgr_reg_chan_change_callback() - Callback to be
 * invoked by regulatory module when valid channel list changes
 * @psoc: PSOC object information
 * @pdev: PDEV object information
 * @chan_list: New channel list
 * @avoid_freq_ind: LTE coex avoid channel list
 * @arg: Information passed at registration
 *
 * Get updated channel list from regulatory module
 *
 * Return: None
 */
void policy_mgr_reg_chan_change_callback(struct wlan_objmgr_psoc *psoc,
		struct wlan_objmgr_pdev *pdev,
		struct regulatory_channel *chan_list,
		struct avoid_freq_ind_data *avoid_freq_ind,
		void *arg);

/**
 * policy_mgr_nss_update() - update nss for AP vdev
 * @psoc: PSOC object information
 * @new_nss: new NSS value
 * @next_action: Next action after nss update
 * @band: update AP vdev on the Band.
 * @reason: action reason
 * @original_vdev_id: original request hwmode change vdev id
 *
 * The function will update AP vdevs on specific band.
 *  eg. band = POLICY_MGR_ANY will request to update all band (2g and 5g)
 *
 * Return: QDF_STATUS_SUCCESS, update requested successfully.
 */
QDF_STATUS policy_mgr_nss_update(struct wlan_objmgr_psoc *psoc,
		uint8_t  new_nss, uint8_t next_action,
		enum policy_mgr_band band,
		enum policy_mgr_conn_update_reason reason,
		uint32_t original_vdev_id);

/**
 * policy_mgr_is_concurrency_allowed() - Check for allowed
 * concurrency combination
 * @psoc: PSOC object information
 * @mode: new connection mode
 * @channel: channel on which new connection is coming up
 * @bw: Bandwidth requested by the connection (optional)
 *
 * When a new connection is about to come up check if current
 * concurrency combination including the new connection is
 * allowed or not based on the HW capability, but no need to
 * invoke get_pcl
 *
 * Return: True/False
 */
bool policy_mgr_is_concurrency_allowed(struct wlan_objmgr_psoc *psoc,
				       enum policy_mgr_con_mode mode,
				       uint8_t channel,
				       enum hw_mode_bandwidth bw);
#endif
