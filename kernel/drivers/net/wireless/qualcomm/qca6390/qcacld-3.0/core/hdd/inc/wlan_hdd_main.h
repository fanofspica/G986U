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

#if !defined(WLAN_HDD_MAIN_H)
#define WLAN_HDD_MAIN_H
/**
 * DOC: wlan_hdd_main.h
 *
 * Linux HDD Adapter Type
 */

/*
 * The following terms were in use in prior versions of the driver but
 * have now been replaced with terms that are aligned with the Linux
 * Coding style. Macros are defined to hopefully prevent new instances
 * from being introduced, primarily by code propagation.
 */
#define pHddCtx
#define pAdapter
#define pHostapdAdapter
#define pHddApCtx
#define pHddStaCtx
#define pHostapdState
#define pRoamInfo
#define pScanInfo
#define pBeaconIes

/*
 * Include files
 */

#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <net/cfg80211.h>
#ifdef CLD_PM_QOS
#include <linux/pm_qos.h>
#endif
#include <linux/ieee80211.h>
#include <qdf_delayed_work.h>
#include <qdf_list.h>
#include <qdf_types.h>
#include "sir_mac_prot_def.h"
#include "csr_api.h"
#include "wlan_dsc.h"
#include <wlan_hdd_assoc.h>
#include <wlan_hdd_wmm.h>
#include <wlan_hdd_cfg.h>
#include <linux/spinlock.h>
#if defined(WLAN_OPEN_SOURCE) && defined(CONFIG_HAS_WAKELOCK)
#include <linux/wakelock.h>
#endif
#ifdef WLAN_FEATURE_TSF_PTP
#include <linux/ptp_classify.h>
#include <linux/ptp_clock_kernel.h>
#endif
#include <wlan_hdd_ftm.h>
#include "wlan_hdd_tdls.h"
#include "wlan_hdd_tsf.h"
#include "wlan_hdd_cfg80211.h"
#include "wlan_hdd_debugfs.h"
#include <qdf_defer.h>
#include "sap_api.h"
#include <wlan_hdd_lro.h>
#include "cdp_txrx_flow_ctrl_legacy.h"
#include <cdp_txrx_peer_ops.h>
#include "wlan_hdd_nan_datapath.h"
#if defined(CONFIG_HL_SUPPORT)
#include "wlan_tgt_def_config_hl.h"
#else
#include "wlan_tgt_def_config.h"
#endif
#include <wlan_objmgr_cmn.h>
#include <wlan_objmgr_global_obj.h>
#include <wlan_objmgr_psoc_obj.h>
#include <wlan_objmgr_pdev_obj.h>
#include <wlan_objmgr_vdev_obj.h>
#include <wlan_objmgr_peer_obj.h>
#include "wlan_pmo_ucfg_api.h"
#ifdef WIFI_POS_CONVERGED
#include "os_if_wifi_pos.h"
#include "wifi_pos_api.h"
#else
#include "wlan_hdd_oemdata.h"
#endif
#include "wlan_hdd_he.h"

#include <net/neighbour.h>
#include <net/netevent.h>
#include "wlan_hdd_nud_tracking.h"
#include "wlan_hdd_twt.h"
#include "wma_sar_public_structs.h"
#include "wlan_mlme_ucfg_api.h"

#ifdef WLAN_FEATURE_DP_BUS_BANDWIDTH
#include "qdf_periodic_work.h"
#endif

/*
 * Preprocessor definitions and constants
 */

#ifdef FEATURE_WLAN_APF
/**
 * struct hdd_apf_context - hdd Context for apf
 * @magic: magic number
 * @qdf_apf_event: Completion variable for APF get operations
 * @capability_response: capabilities response received from fw
 * @apf_enabled: True: APF Interpreter enabled, False: Disabled
 * @cmd_in_progress: Flag that indicates an APF command is in progress
 * @buf: Buffer to accumulate read memory chunks
 * @buf_len: Length of the read memory requested
 * @offset: APF work memory offset to fetch from
 * @lock: APF Context lock
 */
struct hdd_apf_context {
	unsigned int magic;
	qdf_event_t qdf_apf_event;
	bool apf_enabled;
	bool cmd_in_progress;
	uint8_t *buf;
	uint32_t buf_len;
	uint32_t offset;
	qdf_spinlock_t lock;
};
#endif /* FEATURE_WLAN_APF */

/** Number of Tx Queues */
#if defined(QCA_LL_TX_FLOW_CONTROL_V2) || defined(QCA_HL_NETDEV_FLOW_CONTROL)
#define NUM_TX_QUEUES 5
#else
#define NUM_TX_QUEUES 4
#endif

/*
 * API in_compat_syscall() is introduced in 4.6 kernel to check whether we're
 * in a compat syscall or not. It is a new way to query the syscall type, which
 * works properly on all architectures.
 *
 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 6, 0))
static inline bool in_compat_syscall(void) { return is_compat_task(); }
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)) || \
	defined(CFG80211_REMOVE_IEEE80211_BACKPORT)
#define HDD_NL80211_BAND_2GHZ   NL80211_BAND_2GHZ
#define HDD_NL80211_BAND_5GHZ   NL80211_BAND_5GHZ
#define HDD_NUM_NL80211_BANDS   NUM_NL80211_BANDS
#else
#define HDD_NL80211_BAND_2GHZ   IEEE80211_BAND_2GHZ
#define HDD_NL80211_BAND_5GHZ   IEEE80211_BAND_5GHZ
#define HDD_NUM_NL80211_BANDS   ((enum nl80211_band)IEEE80211_NUM_BANDS)
#endif

#define TSF_GPIO_PIN_INVALID 255

/** Length of the TX queue for the netdev */
#define HDD_NETDEV_TX_QUEUE_LEN (3000)

/** Hdd Tx Time out value */
#define HDD_TX_TIMEOUT          msecs_to_jiffies(5000)

#define HDD_TX_STALL_THRESHOLD 4

/** Hdd Default MTU */
#define HDD_DEFAULT_MTU         (1500)

#ifdef QCA_CONFIG_SMP
#define NUM_CPUS NR_CPUS
#else
#define NUM_CPUS 1
#endif

/**
 * enum hdd_adapter_flags - event bitmap flags registered net device
 * @NET_DEVICE_REGISTERED: Adapter is registered with the kernel
 * @SME_SESSION_OPENED: Firmware vdev has been created
 * @INIT_TX_RX_SUCCESS: Adapter datapath is initialized
 * @WMM_INIT_DONE: Adapter is initialized
 * @SOFTAP_BSS_STARTED: Software Access Point (SAP) is running
 * @DEVICE_IFACE_OPENED: Adapter has been "opened" via the kernel
 * @ACS_PENDING: Auto Channel Selection (ACS) is pending
 * @SOFTAP_INIT_DONE: Software Access Point (SAP) is initialized
 * @VENDOR_ACS_RESPONSE_PENDING: Waiting for event for vendor acs
 * @DOWN_DURING_SSR: Mark interface is down during SSR
 */
enum hdd_adapter_flags {
	NET_DEVICE_REGISTERED,
	SME_SESSION_OPENED,
	INIT_TX_RX_SUCCESS,
	WMM_INIT_DONE,
	SOFTAP_BSS_STARTED,
	DEVICE_IFACE_OPENED,
	ACS_PENDING,
	SOFTAP_INIT_DONE,
	VENDOR_ACS_RESPONSE_PENDING,
	DOWN_DURING_SSR,
};

/**
 * enum hdd_driver_flags - HDD global event bitmap flags
 * @ACS_IN_PROGRESS: Auto Channel Selection (ACS) in progress
 */
enum hdd_driver_flags {
	ACS_IN_PROGRESS,
};

#define WLAN_WAIT_DISCONNECT_ALREADY_IN_PROGRESS  1000
#define WLAN_WAIT_TIME_STOP_ROAM  4000
#define WLAN_WAIT_TIME_STATS       800
#define WLAN_WAIT_TIME_LINK_STATUS 800

/** Maximum time(ms) to wait for mc thread suspend **/
#define WLAN_WAIT_TIME_MCTHREAD_SUSPEND  1200

/** Maximum time(ms) to wait for target to be ready for suspend **/
#define WLAN_WAIT_TIME_READY_TO_SUSPEND  2000

/* Scan Req Timeout */
#define WLAN_WAIT_TIME_SCAN_REQ 100

#define WLAN_WAIT_TIME_APF     1000

#define WLAN_WAIT_TIME_FW_ROAM_STATS 1000

#define WLAN_WAIT_TIME_ANTENNA_ISOLATION 8000

/* Maximum time(ms) to wait for RSO CMD status event */
#define WAIT_TIME_RSO_CMD_STATUS 2000

/* rcpi request timeout in milli seconds */
#define WLAN_WAIT_TIME_RCPI 500

#define MAX_CFG_STRING_LEN  255

/* Maximum time(ms) to wait for external acs response */
#define WLAN_VENDOR_ACS_WAIT_TIME 1000

/* Mac Address string length */
#define MAC_ADDRESS_STR_LEN 18  /* Including null terminator */
/* Max and min IEs length in bytes */
#define MAX_GENIE_LEN (512)
#define MIN_GENIE_LEN (2)

#define WPS_OUI_TYPE   "\x00\x50\xf2\x04"
#define WPS_OUI_TYPE_SIZE  4

#define P2P_OUI_TYPE   "\x50\x6f\x9a\x09"
#define P2P_OUI_TYPE_SIZE  4

#define HS20_OUI_TYPE   "\x50\x6f\x9a\x10"
#define HS20_OUI_TYPE_SIZE  4

#define OSEN_OUI_TYPE   "\x50\x6f\x9a\x12"
#define OSEN_OUI_TYPE_SIZE  4

#ifdef WLAN_FEATURE_WFD
#define WFD_OUI_TYPE   "\x50\x6f\x9a\x0a"
#define WFD_OUI_TYPE_SIZE  4
#endif

#define MBO_OUI_TYPE   "\x50\x6f\x9a\x16"
#define MBO_OUI_TYPE_SIZE  4

#define QCN_OUI_TYPE   "\x8c\xfd\xf0\x01"
#define QCN_OUI_TYPE_SIZE  4

#define wlan_hdd_get_wps_ie_ptr(ie, ie_len) \
	wlan_get_vendor_ie_ptr_from_oui(WPS_OUI_TYPE, WPS_OUI_TYPE_SIZE, \
	ie, ie_len)

#define hdd_alert(params...) QDF_TRACE_FATAL(QDF_MODULE_ID_HDD, params)
#define hdd_err(params...) QDF_TRACE_ERROR(QDF_MODULE_ID_HDD, params)
#define hdd_warn(params...) QDF_TRACE_WARN(QDF_MODULE_ID_HDD, params)
#define hdd_info(params...) QDF_TRACE_INFO(QDF_MODULE_ID_HDD, params)
#define hdd_debug(params...) QDF_TRACE_DEBUG(QDF_MODULE_ID_HDD, params)

#define hdd_nofl_alert(params...) \
	QDF_TRACE_FATAL_NO_FL(QDF_MODULE_ID_HDD, params)
#define hdd_nofl_err(params...) \
	QDF_TRACE_ERROR_NO_FL(QDF_MODULE_ID_HDD, params)
#define hdd_nofl_warn(params...) \
	QDF_TRACE_WARN_NO_FL(QDF_MODULE_ID_HDD, params)
#define hdd_nofl_info(params...) \
	QDF_TRACE_INFO_NO_FL(QDF_MODULE_ID_HDD, params)
#define hdd_nofl_debug(params...) \
	QDF_TRACE_DEBUG_NO_FL(QDF_MODULE_ID_HDD, params)

#define hdd_alert_rl(params...) QDF_TRACE_FATAL_RL(QDF_MODULE_ID_HDD, params)
#define hdd_err_rl(params...) QDF_TRACE_ERROR_RL(QDF_MODULE_ID_HDD, params)
#define hdd_warn_rl(params...) QDF_TRACE_WARN_RL(QDF_MODULE_ID_HDD, params)
#define hdd_info_rl(params...) QDF_TRACE_INFO_RL(QDF_MODULE_ID_HDD, params)
#define hdd_debug_rl(params...) QDF_TRACE_DEBUG_RL(QDF_MODULE_ID_HDD, params)

#define hdd_enter() QDF_TRACE_ENTER(QDF_MODULE_ID_HDD, "enter")
#define hdd_enter_dev(dev) \
	QDF_TRACE_ENTER(QDF_MODULE_ID_HDD, "enter(%s)", (dev)->name)
#define hdd_exit() QDF_TRACE_EXIT(QDF_MODULE_ID_HDD, "exit")

#define WLAN_HDD_GET_PRIV_PTR(__dev__) \
		(struct hdd_adapter *)(netdev_priv((__dev__)))

#define MAX_NO_OF_2_4_CHANNELS 14

#define WLAN_HDD_PUBLIC_ACTION_FRAME_OFFSET 24

#define WLAN_HDD_IS_SOCIAL_CHANNEL(center_freq)	\
	(((center_freq) == 2412) || ((center_freq) == 2437) || \
	((center_freq) == 2462))

#define WLAN_HDD_QOS_ACTION_FRAME 1
#define WLAN_HDD_QOS_MAP_CONFIGURE 4
#define HDD_SAP_WAKE_LOCK_DURATION WAKELOCK_DURATION_RECOMMENDED

/* SAP client disconnect wake lock duration in milli seconds */
#define HDD_SAP_CLIENT_DISCONNECT_WAKE_LOCK_DURATION \
	WAKELOCK_DURATION_RECOMMENDED

#define HDD_CFG_REQUEST_FIRMWARE_RETRIES (3)
#define HDD_CFG_REQUEST_FIRMWARE_DELAY (20)

#define MAX_USER_COMMAND_SIZE 4096
#define DNS_DOMAIN_NAME_MAX_LEN 255
#define ICMPv6_ADDR_LEN 16


#define HDD_MIN_TX_POWER (-100) /* minimum tx power */
#define HDD_MAX_TX_POWER (+100) /* maximum tx power */

#define HDD_ENABLE_SIFS_BURST_DEFAULT	(1)
/* If IPA UC data path is enabled, target should reserve extra tx descriptors
 * for IPA data path.
 * Then host data path should allow less TX packet pumping in case
 * IPA data path enabled
 */
#define WLAN_TFC_IPAUC_TX_DESC_RESERVE   100

/*
 * NET_NAME_UNKNOWN is only introduced after Kernel 3.17, to have a macro
 * here if the Kernel version is less than 3.17 to avoid the interleave
 * conditional compilation.
 */
#if !((LINUX_VERSION_CODE >= KERNEL_VERSION(3, 17, 0)) ||\
	defined(WITH_BACKPORTS))
#define NET_NAME_UNKNOWN	0
#endif

#define PRE_CAC_SSID "pre_cac_ssid"

#define SCAN_REJECT_THRESHOLD_TIME 300000 /* Time is in msec, equal to 5 mins */
#define SCAN_REJECT_THRESHOLD 15

/* Default Psoc id */
#define DEFAULT_PSOC_ID 1

/* wait time for nud stats in milliseconds */
#define WLAN_WAIT_TIME_NUD_STATS 800
/* nud stats skb max length */
#define WLAN_NUD_STATS_LEN 800
/* ARP packet type for NUD debug stats */
#define WLAN_NUD_STATS_ARP_PKT_TYPE 1
/* Assigned size of driver memory dump is 4096 bytes */
#define DRIVER_MEM_DUMP_SIZE    4096

/* MAX OS Q block time value in msec
 * Prevent from permanent stall, resume OS Q if timer expired
 */
#define WLAN_HDD_TX_FLOW_CONTROL_OS_Q_BLOCK_TIME 1000
#define WLAN_SAP_HDD_TX_FLOW_CONTROL_OS_Q_BLOCK_TIME 100
#define WLAN_HDD_TX_FLOW_CONTROL_MAX_24BAND_CH   14

#ifndef NUM_TX_RX_HISTOGRAM
#define NUM_TX_RX_HISTOGRAM 128
#endif

#define NUM_TX_RX_HISTOGRAM_MASK (NUM_TX_RX_HISTOGRAM - 1)

/**
 * enum hdd_auth_key_mgmt - auth key mgmt protocols
 * @HDD_AUTH_KEY_MGMT_802_1X: 802.1x
 * @HDD_AUTH_KEY_MGMT_PSK: PSK
 * @HDD_AUTH_KEY_MGMT_CCKM: CCKM
 */
enum hdd_auth_key_mgmt {
	HDD_AUTH_KEY_MGMT_802_1X = BIT(0),
	HDD_AUTH_KEY_MGMT_PSK = BIT(1),
	HDD_AUTH_KEY_MGMT_CCKM = BIT(2)
};

/**
 * struct hdd_tx_rx_histogram - structure to keep track of tx and rx packets
 *				received over 100ms intervals
 * @interval_rx:	# of rx packets received in the last 100ms interval
 * @interval_tx:	# of tx packets received in the last 100ms interval
 * @next_vote_level:	pld_bus_width_type voting level (high or low)
 *			determined on the basis of total tx and rx packets
 *			received in the last 100ms interval
 * @next_rx_level:	pld_bus_width_type voting level (high or low)
 *			determined on the basis of rx packets received in the
 *			last 100ms interval
 * @next_tx_level:	pld_bus_width_type voting level (high or low)
 *			determined on the basis of tx packets received in the
 *			last 100ms interval
 * @qtime		timestamp when the record is added
 *
 * The structure keeps track of throughput requirements of wlan driver.
 * An entry is added if either of next_vote_level, next_rx_level or
 * next_tx_level changes. An entry is not added for every 100ms interval.
 */
struct hdd_tx_rx_histogram {
	uint64_t interval_rx;
	uint64_t interval_tx;
	uint32_t next_vote_level;
	uint32_t next_rx_level;
	uint32_t next_tx_level;
	bool rx_pm_qos_high;
	bool tx_pm_qos_high;
	uint64_t qtime;
};

struct hdd_tx_rx_stats {
	/* start_xmit stats */
	__u32    tx_called;
	__u32    tx_dropped;
	__u32    tx_orphaned;
	__u32    tx_classified_ac[NUM_TX_QUEUES];
	__u32    tx_dropped_ac[NUM_TX_QUEUES];

	/* rx stats */
	__u32 rx_packets[NUM_CPUS];
	__u32 rx_dropped[NUM_CPUS];
	__u32 rx_delivered[NUM_CPUS];
	__u32 rx_refused[NUM_CPUS];
	qdf_atomic_t rx_usolict_arp_n_mcast_drp;

	/* rx gro */
	__u32 rx_aggregated;
	__u32 rx_gro_dropped;
	__u32 rx_non_aggregated;

	/* txflow stats */
	bool     is_txflow_paused;
	__u32    txflow_pause_cnt;
	__u32    txflow_unpause_cnt;
	__u32    txflow_timer_cnt;

	/*tx timeout stats*/
	__u32 tx_timeout_cnt;
	__u32 cont_txtimeout_cnt;
	u64 jiffies_last_txtimeout;
};

#ifdef WLAN_FEATURE_11W
/**
 * struct hdd_pmf_stats - Protected Management Frame statistics
 * @num_unprot_deauth_rx: Number of unprotected deauth frames received
 * @num_unprot_disassoc_rx: Number of unprotected disassoc frames received
 */
struct hdd_pmf_stats {
	uint8_t num_unprot_deauth_rx;
	uint8_t num_unprot_disassoc_rx;
};
#endif

/**
 * struct hdd_arp_stats_s - arp debug stats count
 * @tx_arp_req_count: no. of arp req received from network stack
 * @rx_arp_rsp_count: no. of arp res received from FW
 * @tx_dropped: no. of arp req dropped at hdd layer
 * @rx_dropped: no. of arp res dropped
 * @rx_delivered: no. of arp res delivered to network stack
 * @rx_refused: no of arp rsp refused (not delivered) to network stack
 * @tx_host_fw_sent: no of arp req sent by FW OTA
 * @rx_host_drop_reorder: no of arp res dropped by host
 * @rx_fw_cnt: no of arp res received by FW
 * @tx_ack_cnt: no of arp req acked by FW
 */
struct hdd_arp_stats_s {
	uint16_t tx_arp_req_count;
	uint16_t rx_arp_rsp_count;
	uint16_t tx_dropped;
	uint16_t rx_dropped;
	uint16_t rx_delivered;
	uint16_t rx_refused;
	uint16_t tx_host_fw_sent;
	uint16_t rx_host_drop_reorder;
	uint16_t rx_fw_cnt;
	uint16_t tx_ack_cnt;
};

/**
 * struct hdd_dns_stats_s - dns debug stats count
 * @tx_dns_req_count: no. of dns query received from network stack
 * @rx_dns_rsp_count: no. of dns res received from FW
 * @tx_dropped: no. of dns query dropped at hdd layer
 * @rx_delivered: no. of dns res delivered to network stack
 * @rx_refused: no of dns res refused (not delivered) to network stack
 * @tx_host_fw_sent: no of dns query sent by FW OTA
 * @rx_host_drop: no of dns res dropped by host
 * @tx_ack_cnt: no of dns req acked by FW
 */
struct hdd_dns_stats_s {
	uint16_t tx_dns_req_count;
	uint16_t rx_dns_rsp_count;
	uint16_t tx_dropped;
	uint16_t rx_delivered;
	uint16_t rx_refused;
	uint16_t tx_host_fw_sent;
	uint16_t rx_host_drop;
	uint16_t tx_ack_cnt;
};

/**
 * struct hdd_tcp_stats_s - tcp debug stats count
 * @tx_tcp_syn_count: no. of tcp syn received from network stack
 * @@tx_tcp_ack_count: no. of tcp ack received from network stack
 * @rx_tcp_syn_ack_count: no. of tcp syn ack received from FW
 * @tx_tcp_syn_dropped: no. of tcp syn dropped at hdd layer
 * @tx_tcp_ack_dropped: no. of tcp ack dropped at hdd layer
 * @rx_delivered: no. of tcp syn ack delivered to network stack
 * @rx_refused: no of tcp syn ack refused (not delivered) to network stack
 * @tx_tcp_syn_host_fw_sent: no of tcp syn sent by FW OTA
 * @@tx_tcp_ack_host_fw_sent: no of tcp ack sent by FW OTA
 * @rx_host_drop: no of tcp syn ack dropped by host
 * @tx_tcp_syn_ack_cnt: no of tcp syn acked by FW
 * @tx_tcp_syn_ack_cnt: no of tcp ack acked by FW
 * @is_tcp_syn_ack_rcv: flag to check tcp syn ack received or not
 * @is_tcp_ack_sent: flag to check tcp ack sent or not
 */
struct hdd_tcp_stats_s {
	uint16_t tx_tcp_syn_count;
	uint16_t tx_tcp_ack_count;
	uint16_t rx_tcp_syn_ack_count;
	uint16_t tx_tcp_syn_dropped;
	uint16_t tx_tcp_ack_dropped;
	uint16_t rx_delivered;
	uint16_t rx_refused;
	uint16_t tx_tcp_syn_host_fw_sent;
	uint16_t tx_tcp_ack_host_fw_sent;
	uint16_t rx_host_drop;
	uint16_t rx_fw_cnt;
	uint16_t tx_tcp_syn_ack_cnt;
	uint16_t tx_tcp_ack_ack_cnt;
	bool is_tcp_syn_ack_rcv;
	bool is_tcp_ack_sent;

};

/**
 * struct hdd_icmpv4_stats_s - icmpv4 debug stats count
 * @tx_icmpv4_req_count: no. of icmpv4 req received from network stack
 * @rx_icmpv4_rsp_count: no. of icmpv4 res received from FW
 * @tx_dropped: no. of icmpv4 req dropped at hdd layer
 * @rx_delivered: no. of icmpv4 res delivered to network stack
 * @rx_refused: no of icmpv4 res refused (not delivered) to network stack
 * @tx_host_fw_sent: no of icmpv4 req sent by FW OTA
 * @rx_host_drop: no of icmpv4 res dropped by host
 * @rx_fw_cnt: no of icmpv4 res received by FW
 * @tx_ack_cnt: no of icmpv4 req acked by FW
 */
struct hdd_icmpv4_stats_s {
	uint16_t tx_icmpv4_req_count;
	uint16_t rx_icmpv4_rsp_count;
	uint16_t tx_dropped;
	uint16_t rx_delivered;
	uint16_t rx_refused;
	uint16_t tx_host_fw_sent;
	uint16_t rx_host_drop;
	uint16_t rx_fw_cnt;
	uint16_t tx_ack_cnt;
};

/**
 * struct hdd_peer_stats - Peer stats at HDD level
 * @rx_count: RX count
 * @rx_bytes: RX bytes
 * @fcs_count: FCS err count
 */
struct hdd_peer_stats {
	uint32_t rx_count;
	uint64_t rx_bytes;
	uint32_t fcs_count;
};

struct hdd_stats {
	tCsrSummaryStatsInfo summary_stat;
	tCsrGlobalClassAStatsInfo class_a_stat;
	tCsrGlobalClassDStatsInfo class_d_stat;
	struct csr_per_chain_rssi_stats_info  per_chain_rssi_stats;
	struct hdd_tx_rx_stats tx_rx_stats;
	struct hdd_arp_stats_s hdd_arp_stats;
	struct hdd_dns_stats_s hdd_dns_stats;
	struct hdd_tcp_stats_s hdd_tcp_stats;
	struct hdd_icmpv4_stats_s hdd_icmpv4_stats;
	struct hdd_peer_stats peer_stats;
#ifdef WLAN_FEATURE_11W
	struct hdd_pmf_stats hdd_pmf_stats;
#endif
};

/**
 * struct hdd_roaming_info - HDD Internal Roaming Information
 * @bssid: BSSID to which we are connected
 * @peer_mac: Peer MAC address for IBSS connection
 * @roam_id: Unique identifier for a roaming instance
 * @roam_status: Current roam command status
 * @defer_key_complete: Should key complete be deferred?
 *
 */
struct hdd_roaming_info {
	tSirMacAddr bssid;
	tSirMacAddr peer_mac;
	uint32_t roam_id;
	eRoamCmdStatus roam_status;
	bool defer_key_complete;

};

#ifdef FEATURE_WLAN_WAPI
/* Define WAPI macros for Length, BKID count etc*/
#define MAX_NUM_AKM_SUITES    16

/** WAPI AUTH mode definition */
enum wapi_auth_mode {
	WAPI_AUTH_MODE_OPEN = 0,
	WAPI_AUTH_MODE_PSK = 1,
	WAPI_AUTH_MODE_CERT
} __packed;

#define WPA_GET_LE16(a) ((u16) (((a)[1] << 8) | (a)[0]))
#define WPA_GET_BE24(a) ((u32) ((a[0] << 16) | (a[1] << 8) | a[2]))
#define WAPI_PSK_AKM_SUITE  0x02721400
#define WAPI_CERT_AKM_SUITE 0x01721400

/**
 * struct hdd_wapi_info - WAPI Information structure definition
 * @wapi_mode: Is WAPI enabled on this adapter?
 * @is_wapi_sta: Is the STA associated with WAPI?
 * @wapi_auth_mode: WAPI authentication mode used by this adapter
 */
struct hdd_wapi_info {
	bool wapi_mode;
	bool is_wapi_sta;
	enum wapi_auth_mode wapi_auth_mode;
};
#endif /* FEATURE_WLAN_WAPI */

struct hdd_beacon_data {
	u8 *head;
	u8 *tail;
	u8 *proberesp_ies;
	u8 *assocresp_ies;
	int head_len;
	int tail_len;
	int proberesp_ies_len;
	int assocresp_ies_len;
	int dtim_period;
};

/**
 * struct hdd_mon_set_ch_info - Holds monitor mode channel switch params
 * @channel: Channel number.
 * @cb_mode: Channel bonding
 * @channel_width: Channel width 0/1/2 for 20/40/80MHz respectively.
 * @phy_mode: PHY mode
 */
struct hdd_mon_set_ch_info {
	uint8_t channel;
	uint8_t cb_mode;
	uint32_t channel_width;
	eCsrPhyMode phy_mode;
};

/**
 * struct hdd_station_ctx -- STA-specific information
 * @roam_profile: current roaming profile
 * @security_ie: WPA or RSN IE used by the @roam_profile
 * @assoc_additional_ie: association additional IE used by the @roam_profile
 * @wpa_versions: bitmap of supported WPA versions
 * @auth_key_mgmt: bitmap of supported auth key mgmt protocols
 * @requested_bssid: Specific BSSID to which to connect
 * @conn_info: current connection information
 * @roam_info: current roaming information
 * @ft_carrier_on: is carrier on
 * @ibss_sta_generation: current ibss generation. Incremented whenever
 *    ibss New peer joins and departs the network
 * @ibss_enc_key_installed: is the ibss wep/wpa-none encryptions key
 *    installed?
 * @ibss_enc_key: current ibss wep/wpa-none encryption key (if
 *    @ibss_enc_key_installed is %true)
 * @ibss_peer_info: information about the ibss peer
 * @hdd_reassoc_scenario: is station in the middle of reassociation?
 * @sta_debug_state: STA context debug variable
 * @broadcast_sta_id: STA ID assigned for broadcast frames
 * @ch_info: monitor mode channel information
 * @ap_supports_immediate_power_save: Does the current AP allow our STA
 *    to immediately go into power save?
 */
struct hdd_station_ctx {
	struct csr_roam_profile roam_profile;
	uint8_t security_ie[WLAN_MAX_IE_LEN];
	tSirAddie assoc_additional_ie;
	enum nl80211_wpa_versions wpa_versions;
	enum hdd_auth_key_mgmt auth_key_mgmt;
	struct qdf_mac_addr requested_bssid;
	struct hdd_connection_info conn_info;
	struct hdd_connection_info cache_conn_info;
	struct hdd_roaming_info roam_info;
	int ft_carrier_on;
	int ibss_sta_generation;
	bool ibss_enc_key_installed;
	tCsrRoamSetKey ibss_enc_key;
	tSirPeerInfoRspParams ibss_peer_info;
	bool hdd_reassoc_scenario;
	int sta_debug_state;
	uint8_t broadcast_sta_id;
	struct hdd_mon_set_ch_info ch_info;
	bool ap_supports_immediate_power_save;
};

/**
 * enum bss_state - current state of the BSS
 * @BSS_STOP: BSS is stopped
 * @BSS_START: BSS is started
 */
enum bss_state {
	BSS_STOP,
	BSS_START,
};

/**
 * struct hdd_hostapd_state - hostapd-related state information
 * @bss_state: Current state of the BSS
 * @qdf_event: Event to synchronize actions between hostapd thread and
 *    internal callback threads
 * @qdf_stop_bss_event: Event to synchronize Stop BSS. When Stop BSS
 *    is issued userspace thread can wait on this event. The event will
 *    be set when the Stop BSS processing in UMAC has completed.
 * @qdf_sta_disassoc_event: Event to synchronize STA Disassociation.
 *    When a STA is disassociated userspace thread can wait on this
 *    event. The event will be set when the STA Disassociation
 *    processing in UMAC has completed.
 * @qdf_status: Used to communicate state from other threads to the
 *    userspace thread.
 */
struct hdd_hostapd_state {
	enum bss_state bss_state;
	qdf_event_t qdf_event;
	qdf_event_t qdf_stop_bss_event;
	qdf_event_t qdf_sta_disassoc_event;
	QDF_STATUS qdf_status;
};

/**
 * enum bss_stop_reason - reasons why a BSS is stopped.
 * @BSS_STOP_REASON_INVALID: no reason specified explicitly.
 * @BSS_STOP_DUE_TO_MCC_SCC_SWITCH: BSS stopped due to host
 *  driver is trying to switch AP role to a different channel
 *  to maintain SCC mode with the STA role on the same card.
 *  this usually happens when STA is connected to an external
 *  AP that runs on a different channel
 * @BSS_STOP_DUE_TO_VENDOR_CONFIG_CHAN: BSS stopped due to
 *  vendor subcmd set sap config channel
 */
enum bss_stop_reason {
	BSS_STOP_REASON_INVALID = 0,
	BSS_STOP_DUE_TO_MCC_SCC_SWITCH = 1,
	BSS_STOP_DUE_TO_VENDOR_CONFIG_CHAN = 2,
};

/**
 * struct hdd_rate_info - rate_info in HDD
 * @rate: tx/rx rate (kbps)
 * @mode: 0->11abg legacy, 1->HT, 2->VHT (refer to sir_sme_phy_mode)
 * @nss: number of streams
 * @mcs: mcs index for HT/VHT mode
 * @rate_flags: rate flags for last tx/rx
 *
 * rate info in HDD
 */
struct hdd_rate_info {
	uint32_t rate;
	uint8_t mode;
	uint8_t nss;
	uint8_t mcs;
	uint8_t rate_flags;
};

/**
 * struct hdd_fw_txrx_stats - fw txrx status in HDD
 *                            (refer to station_info struct in Kernel)
 * @tx_packets: packets transmitted to this station
 * @tx_bytes: bytes transmitted to this station
 * @rx_packets: packets received from this station
 * @rx_bytes: bytes received from this station
 * @rx_retries: cumulative retry counts
 * @tx_failed: number of failed transmissions
 * @rssi: The signal strength (dbm)
 * @tx_rate: last used tx rate info
 * @rx_rate: last used rx rate info
 *
 * fw txrx status in HDD
 */
struct hdd_fw_txrx_stats {
	uint32_t tx_packets;
	uint64_t tx_bytes;
	uint32_t rx_packets;
	uint64_t rx_bytes;
	uint32_t tx_retries;
	uint32_t tx_failed;
	int8_t rssi;
	struct hdd_rate_info tx_rate;
	struct hdd_rate_info rx_rate;
};

/**
 * struct dhcp_phase - Per Peer DHCP Phases
 * @DHCP_PHASE_ACK: upon receiving DHCP_ACK/NAK message in REQUEST phase or
 *         DHCP_DELINE message in OFFER phase
 * @DHCP_PHASE_DISCOVER: upon receiving DHCP_DISCOVER message in ACK phase
 * @DHCP_PHASE_OFFER: upon receiving DHCP_OFFER message in DISCOVER phase
 * @DHCP_PHASE_REQUEST: upon receiving DHCP_REQUEST message in OFFER phase or
 *         ACK phase (Renewal process)
 */
enum dhcp_phase {
	DHCP_PHASE_ACK,
	DHCP_PHASE_DISCOVER,
	DHCP_PHASE_OFFER,
	DHCP_PHASE_REQUEST
};

/**
 * struct dhcp_nego_status - Per Peer DHCP Negotiation Status
 * @DHCP_NEGO_STOP: when the peer is in ACK phase or client disassociated
 * @DHCP_NEGO_IN_PROGRESS: when the peer is in DISCOVER or REQUEST
 *         (Renewal process) phase
 */
enum dhcp_nego_status {
	DHCP_NEGO_STOP,
	DHCP_NEGO_IN_PROGRESS
};

/**
 * struct hdd_station_info - Per station structure kept in HDD for
 *                                     multiple station support for SoftAP
 * @in_use: Is the station entry in use?
 * @sta_id: Station ID reported back from HAL (through SAP).
 *           Broadcast uses station ID zero by default.
 * @sta_type: Type of station i.e. p2p client or infrastructure station
 * @sta_mac: MAC address of the station
 * @peer_state: Current Station state so HDD knows how to deal with packet
 *              queue. Most recent states used to change TLSHIM STA state.
 * @is_qos_enabled: Track QoS status of station
 * @is_deauth_in_progress: The station entry for which Deauth is in progress
 * @nss: Number of spatial streams supported
 * @rate_flags: Rate Flags for this connection
 * @ecsa_capable: Extended CSA capabilities
 * @max_phy_rate: Calcuated maximum phy rate based on mode, nss, mcs etc.
 * @tx_packets: Packets send to current station
 * @tx_bytes: Bytes send to current station
 * @rx_packets: Packets received from current station
 * @rx_bytes: Bytes received from current station
 * @last_tx_rx_ts: Last tx/rx timestamp with current station
 * @assoc_ts: Current station association timestamp
 * @tx_rate: Tx rate with current station reported from F/W
 * @rx_rate: Rx rate with current station reported from F/W
 * @ampdu: Ampdu enable or not of the station
 * @sgi_enable: Short GI enable or not of the station
 * @tx_stbc: Tx Space-time block coding enable/disable
 * @rx_stbc: Rx Space-time block coding enable/disable
 * @ch_width: Channel Width of the connection
 * @mode: Mode of the connection
 * @max_supp_idx: Max supported rate index of the station
 * @max_ext_idx: Max extended supported rate index of the station
 * @max_mcs_idx: Max supported mcs index of the station
 * @rx_mcs_map: VHT Rx mcs map
 * @tx_mcs_map: VHT Tx mcs map
 * @freq : Frequency of the current station
 * @dot11_mode: 802.11 Mode of the connection
 * @ht_present: HT caps present or not in the current station
 * @vht_present: VHT caps present or not in the current station
 * @ht_caps: HT capabilities of current station
 * @vht_caps: VHT capabilities of current station
 * @reason_code: Disconnection reason code for current station
 * @rssi: RSSI of the current station reported from F/W
 * @capability: Capability information of current station
 * @support_mode: Max supported mode of a station currently
 * connected to sap
 * @rx_retry_cnt: Number of rx retries received from current station
 *                Currently this feature is not supported from FW
 * @rx_mc_bc_cnt: Multicast broadcast packet count received from
 *                current station
 * MSB of rx_mc_bc_cnt indicates whether FW supports rx_mc_bc_cnt
 * feature or not, if first bit is 1 it indicates that FW supports this
 * feature, if it is 0 it indicates FW doesn't support this feature
 */
struct hdd_station_info {
	bool in_use;
	uint8_t sta_id;
	eStationType sta_type;
	struct qdf_mac_addr sta_mac;
	enum ol_txrx_peer_state peer_state;
	bool is_qos_enabled;
	bool is_deauth_in_progress;
	uint8_t   nss;
	uint32_t  rate_flags;
	uint8_t   ecsa_capable;
	uint32_t max_phy_rate;
	uint32_t tx_packets;
	uint64_t tx_bytes;
	uint32_t rx_packets;
	uint64_t rx_bytes;
	qdf_time_t last_tx_rx_ts;
	qdf_time_t assoc_ts;
	qdf_time_t disassoc_ts;
	uint32_t tx_rate;
	uint32_t rx_rate;
	bool ampdu;
	bool sgi_enable;
	bool tx_stbc;
	bool rx_stbc;
	tSirMacHTChannelWidth ch_width;
	uint8_t mode;
	uint8_t max_supp_idx;
	uint8_t max_ext_idx;
	uint8_t max_mcs_idx;
	uint8_t rx_mcs_map;
	uint8_t tx_mcs_map;
	uint32_t freq;
	uint8_t dot11_mode;
	bool ht_present;
	bool vht_present;
	struct ieee80211_ht_cap ht_caps;
	struct ieee80211_vht_cap vht_caps;
	uint32_t reason_code;
	int8_t rssi;
	enum dhcp_phase dhcp_phase;
	enum dhcp_nego_status dhcp_nego_status;
	uint16_t capability;
	uint8_t support_mode;
	uint32_t rx_retry_cnt;
	uint32_t rx_mc_bc_cnt;
};

/**
 * struct hdd_ap_ctx - SAP/P2PGO specific information
 * @hostapd_state: state control information
 * @dfs_cac_block_tx: Is data tramsmission blocked due to DFS CAC?
 * @ap_active: Are any stations active?
 * @disable_intrabss_fwd: Prevent forwarding between stations
 * @broadcast_sta_id: Station ID assigned after BSS starts
 * @privacy: The privacy bits of configuration
 * @encryption_type: The encryption being used
 * @group_key: Group Encryption Key
 * @wep_key: WEP key array
 * @wep_def_key_idx: WEP default key index
 * @sap_context: Pointer to context maintained by SAP (opaque to HDD)
 * @sap_config: SAP configuration
 * @operating_channel: channel upon which the SAP is operating
 * @beacon: Beacon information
 * @vendor_acs_timer: Timer for ACS
 * @vendor_acs_timer_initialized: Is @vendor_acs_timer initialized?
 * @bss_stop_reason: Reason why the BSS was stopped
 * @acs_in_progress: In progress acs flag for an adapter
 */
struct hdd_ap_ctx {
	struct hdd_hostapd_state hostapd_state;
	bool dfs_cac_block_tx;
	bool ap_active;
	bool disable_intrabss_fwd;
	uint8_t broadcast_sta_id;
	uint8_t privacy;
	eCsrEncryptionType encryption_type;
	tCsrRoamSetKey group_key;
	tCsrRoamSetKey wep_key[CSR_MAX_NUM_KEY];
	uint8_t wep_def_key_idx;
	struct sap_context *sap_context;
	struct sap_config sap_config;
	uint8_t operating_channel;
	struct hdd_beacon_data *beacon;
	qdf_mc_timer_t vendor_acs_timer;
	bool vendor_acs_timer_initialized;
	enum bss_stop_reason bss_stop_reason;
	qdf_atomic_t acs_in_progress;
};

/**
 * struct hdd_scan_info - Per-adapter scan information
 * @scan_add_ie: Additional IE for scan
 * @default_scan_ies: Default scan IEs
 * @default_scan_ies_len: Length of @default_scan_ies
 * @scan_mode: Scan mode
 */
struct hdd_scan_info {
	tSirAddie scan_add_ie;
	uint8_t *default_scan_ies;
	uint16_t default_scan_ies_len;
	tSirScanType scan_mode;
};

#define WLAN_HDD_MAX_MC_ADDR_LIST CFG_TGT_MAX_MULTICAST_FILTER_ENTRIES

struct hdd_multicast_addr_list {
	uint8_t mc_cnt;
	uint8_t addr[WLAN_HDD_MAX_MC_ADDR_LIST][ETH_ALEN];
};

#define WLAN_HDD_MAX_HISTORY_ENTRY 25

/**
 * struct hdd_netif_queue_stats - netif queue operation statistics
 * @pause_count - pause counter
 * @unpause_count - unpause counter
 */
struct hdd_netif_queue_stats {
	u32 pause_count;
	u32 unpause_count;
	qdf_time_t total_pause_time;
};

/**
 * struct hdd_netif_queue_history - netif queue operation history
 * @time: timestamp
 * @netif_action: action type
 * @netif_reason: reason type
 * @pause_map: pause map
 * @tx_q_state: state of the netdev TX queues
 */
struct hdd_netif_queue_history {
	qdf_time_t time;
	uint16_t netif_action;
	uint16_t netif_reason;
	uint32_t pause_map;
	unsigned long tx_q_state[NUM_TX_QUEUES];
};

/**
 * struct hdd_chan_change_params - channel related information
 * @chan: operating channel
 * @chan_params: channel parameters
 */
struct hdd_chan_change_params {
	uint8_t chan;
	struct ch_params chan_params;
};

/**
 * struct hdd_runtime_pm_context - context to prevent/allow runtime pm
 * @dfs: dfs context to prevent/allow runtime pm
 * @connect: connect context to prevent/allow runtime pm
 *
 * Runtime PM control for underlying activities
 */
struct hdd_runtime_pm_context {
	qdf_runtime_lock_t dfs;
	qdf_runtime_lock_t connect;
};

/*
 * WLAN_HDD_ADAPTER_MAGIC is a magic number used to identify net devices
 * belonging to this driver from net devices belonging to other devices.
 * Therefore, the magic number must be unique relative to the numbers for
 * other drivers in the system. If WLAN_HDD_ADAPTER_MAGIC is already defined
 * (e.g. by compiler argument), then use that. If it's not already defined,
 * then use the first 4 characters of MULTI_IF_NAME to construct the magic
 * number. If MULTI_IF_NAME is not defined, then use a default magic number.
 */
#ifndef WLAN_HDD_ADAPTER_MAGIC
#ifdef MULTI_IF_NAME
#define WLAN_HDD_ADAPTER_MAGIC                                          \
	(MULTI_IF_NAME[0] == 0 ? 0x574c414e :                           \
	(MULTI_IF_NAME[1] == 0 ? (MULTI_IF_NAME[0] << 24) :             \
	(MULTI_IF_NAME[2] == 0 ? (MULTI_IF_NAME[0] << 24) |             \
		(MULTI_IF_NAME[1] << 16) :                              \
	(MULTI_IF_NAME[0] << 24) | (MULTI_IF_NAME[1] << 16) |           \
	(MULTI_IF_NAME[2] << 8) | MULTI_IF_NAME[3])))
#else
#define WLAN_HDD_ADAPTER_MAGIC 0x574c414e       /* ASCII "WLAN" */
#endif
#endif

/**
 * struct rcpi_info - rcpi info
 * @rcpi: computed value in dB
 * @mac_addr: peer mac addr for which rcpi is computed
 */
struct rcpi_info {
	int32_t rcpi;
	struct qdf_mac_addr mac_addr;
};

struct hdd_context;

/**
 * struct hdd_adapter - hdd vdev/net_device context
 * @vdev: object manager vdev context
 * @vdev_lock: lock to protect vdev context access
 * @vdev_id: Unique identifier assigned to the vdev
 * @event_flags: a bitmap of hdd_adapter_flags
 */
struct hdd_adapter {
	/* Magic cookie for adapter sanity verification.  Note that this
	 * needs to be at the beginning of the private data structure so
	 * that it will exist at the beginning of dev->priv and hence
	 * will always be in mapped memory
	 */
	uint32_t magic;

	/* list node for membership in the adapter list */
	qdf_list_node_t node;

	struct hdd_context *hdd_ctx;
	struct wlan_objmgr_vdev *vdev;
	qdf_spinlock_t vdev_lock;
	uint8_t vdev_id;

	void *txrx_vdev;

	/** Handle to the network device */
	struct net_device *dev;

	enum QDF_OPMODE device_mode;

	/** IPv4 notifier callback for handling ARP offload on change in IP */
	struct work_struct ipv4_notifier_work;
#ifdef WLAN_NS_OFFLOAD
	/** IPv6 notifier callback for handling NS offload on change in IP */
	struct work_struct ipv6_notifier_work;
#endif

	/* TODO Move this to sta Ctx */
	struct wireless_dev wdev;

	/** ops checks if Opportunistic Power Save is Enable or Not
	 * ctw stores CT Window value once we receive Opps command from
	 * wpa_supplicant then using CT Window value we need to Enable
	 * Opportunistic Power Save
	 */
	uint8_t ops;
	uint32_t ctw;

	/** Current MAC Address for the adapter  */
	struct qdf_mac_addr mac_addr;

#ifdef WLAN_NUD_TRACKING
	struct hdd_nud_tracking_info nud_tracking;
#endif
	bool disconnection_in_progress;
	qdf_mutex_t disconnection_status_lock;
	unsigned long event_flags;

	/**Device TX/RX statistics*/
	struct net_device_stats stats;
	/** HDD statistics*/
	struct hdd_stats hdd_stats;

	/* estimated link speed */
	uint32_t estimated_linkspeed;

	/* QDF event for session close */
	qdf_event_t qdf_session_close_event;

	/* QDF event for session open */
	qdf_event_t qdf_session_open_event;

	/* TODO: move these to sta ctx. These may not be used in AP */
	/** completion variable for disconnect callback */
	struct completion disconnect_comp_var;

	struct completion roaming_comp_var;

	/* completion variable for Linkup Event */
	struct completion linkup_event_var;

	/* completion variable for off channel  remain on channel Event */
	struct completion offchannel_tx_event;
	/* Completion variable for action frame */
	struct completion tx_action_cnf_event;

	struct completion sta_authorized_event;

	struct completion ibss_peer_info_comp;

	/* Track whether the linkup handling is needed  */
	bool is_link_up_service_needed;

	/* WMM Status */
	struct hdd_wmm_status hdd_wmm_status;

	/** Multiple station supports */
	/** Per-station structure */
	spinlock_t sta_info_lock;        /* To protect access to station Info */
	struct hdd_station_info sta_info[WLAN_MAX_STA_COUNT];
	struct hdd_station_info cache_sta_info[WLAN_MAX_STA_COUNT];


#ifdef FEATURE_WLAN_WAPI
	struct hdd_wapi_info wapi_info;
#endif

	int8_t rssi;
	int32_t rssi_on_disconnect;
#ifdef WLAN_FEATURE_LPSS
	bool rssi_send;
#endif

	uint8_t snr;

	struct work_struct  sap_stop_bss_work;

	union {
		struct hdd_station_ctx station;
		struct hdd_ap_ctx ap;
	} session;

	qdf_atomic_t ch_switch_in_progress;

#ifdef WLAN_FEATURE_TSF
	/* tsf value received from firmware */
	uint64_t cur_target_time;
	uint64_t cur_tsf_sync_soc_time;
	uint64_t last_tsf_sync_soc_time;
	uint64_t cur_target_global_tsf_time;
	uint64_t last_target_global_tsf_time;
	qdf_mc_timer_t host_capture_req_timer;
#ifdef WLAN_FEATURE_TSF_PLUS
	/* spin lock for read/write timestamps */
	qdf_spinlock_t host_target_sync_lock;
	qdf_mc_timer_t host_target_sync_timer;
	uint64_t cur_host_time;
	uint64_t last_host_time;
	uint64_t last_target_time;
	/* to store the count of continuous invalid tstamp-pair */
	int continuous_error_count;
	/* to indicate whether tsf_sync has been initialized */
	qdf_atomic_t tsf_sync_ready_flag;
#endif /* WLAN_FEATURE_TSF_PLUS */
#endif

	struct hdd_multicast_addr_list mc_addr_list;
	uint8_t addr_filter_pattern;

	struct hdd_scan_info scan_info;

	/* Flag to ensure PSB is configured through framework */
	uint8_t psb_changed;
	/* UAPSD psb value configured through framework */
	uint8_t configured_psb;
	/* Use delayed work for Sec AP ACS as Pri AP Startup need to complete
	 * since CSR (PMAC Struct) Config is same for both AP
	 */
	struct delayed_work acs_pending_work;

	struct work_struct scan_block_work;
	qdf_list_t blocked_scan_request_q;
	qdf_mutex_t blocked_scan_request_q_lock;

#ifdef WLAN_FEATURE_DP_BUS_BANDWIDTH
	unsigned long prev_rx_packets;
	unsigned long prev_tx_packets;
	uint64_t prev_fwd_tx_packets;
	uint64_t prev_fwd_rx_packets;
#endif /*WLAN_FEATURE_DP_BUS_BANDWIDTH*/

#if  defined(QCA_LL_LEGACY_TX_FLOW_CONTROL) || \
				defined(QCA_HL_NETDEV_FLOW_CONTROL)
	qdf_mc_timer_t tx_flow_control_timer;
	bool tx_flow_timer_initialized;
#endif /* QCA_LL_LEGACY_TX_FLOW_CONTROL || QCA_HL_NETDEV_FLOW_CONTROL */
#ifdef QCA_LL_LEGACY_TX_FLOW_CONTROL
	unsigned int tx_flow_low_watermark;
	unsigned int tx_flow_hi_watermark_offset;
#endif /* QCA_LL_LEGACY_TX_FLOW_CONTROL */

	bool offloads_configured;

	/* DSCP to UP QoS Mapping */
	enum sme_qos_wmmuptype dscp_to_up_map[WLAN_HDD_MAX_DSCP + 1];

#ifdef WLAN_FEATURE_LINK_LAYER_STATS
	bool is_link_layer_stats_set;
#endif
	uint8_t link_status;

	/* variable for temperature in Celsius */
	int temperature;

#ifdef WLAN_FEATURE_DSRC
	/* MAC addresses used for OCB interfaces */
	struct qdf_mac_addr ocb_mac_address[QDF_MAX_CONCURRENCY_PERSONA];
	int ocb_mac_addr_count;
#endif

	/* BITMAP indicating pause reason */
	uint32_t pause_map;
	spinlock_t pause_map_lock;
	qdf_time_t start_time;
	qdf_time_t last_time;
	qdf_time_t total_pause_time;
	qdf_time_t total_unpause_time;
	uint8_t history_index;
	struct hdd_netif_queue_history
		 queue_oper_history[WLAN_HDD_MAX_HISTORY_ENTRY];
	struct hdd_netif_queue_stats queue_oper_stats[WLAN_REASON_TYPE_MAX];
	ol_txrx_tx_fp tx_fn;
	/* debugfs entry */
	struct dentry *debugfs_phy;
	/*
	 * The pre cac channel is saved here and will be used when the SAP's
	 * channel needs to be moved from the existing 2.4GHz channel.
	 */
	uint8_t pre_cac_chan;

	/*
	 * Indicate if HO fails during disconnect so that
	 * disconnect is not initiated by HDD as its already
	 * initiated by CSR
	 */
	bool roam_ho_fail;
	struct lfr_firmware_status lfr_fw_status;
	bool con_status;
	bool dad;
	uint8_t active_ac;
	uint32_t pkt_type_bitmap;
	uint32_t track_arp_ip;
	uint8_t dns_payload[256];
	uint32_t track_dns_domain_len;
	uint32_t track_src_port;
	uint32_t track_dest_port;
	uint32_t track_dest_ipv4;
	uint32_t mon_chan;
	uint32_t mon_bandwidth;

	/* rcpi information */
	struct rcpi_info rcpi;
	bool send_mode_change;
#ifdef FEATURE_WLAN_APF
	struct hdd_apf_context apf_context;
#endif /* FEATURE_WLAN_APF */

#ifdef WLAN_DEBUGFS
	struct hdd_debugfs_file_info csr_file[HDD_DEBUGFS_FILE_ID_MAX];
#endif /* WLAN_DEBUGFS */

#ifdef WLAN_FEATURE_MOTION_DETECTION
	bool motion_detection_mode;
#endif /* WLAN_FEATURE_MOTION_DETECTION */
};

#define WLAN_HDD_GET_STATION_CTX_PTR(adapter) (&(adapter)->session.station)
#define WLAN_HDD_GET_AP_CTX_PTR(adapter) (&(adapter)->session.ap)
#define WLAN_HDD_GET_CTX(adapter) ((adapter)->hdd_ctx)
#define WLAN_HDD_GET_HOSTAP_STATE_PTR(adapter) \
				(&(adapter)->session.ap.hostapd_state)
#define WLAN_HDD_GET_SAP_CTX_PTR(adapter) ((adapter)->session.ap.sap_context)

#ifdef WLAN_FEATURE_NAN
#define WLAN_HDD_IS_NDP_ENABLED(hdd_ctx) ((hdd_ctx)->nan_datapath_enabled)
#else
/* WLAN_HDD_GET_NDP_CTX_PTR and WLAN_HDD_GET_NDP_WEXT_STATE_PTR are not defined
 * intentionally so that all references to these must be within NDP code.
 * non-NDP code can call WLAN_HDD_IS_NDP_ENABLED(), and when it is enabled,
 * invoke NDP code to do all work.
 */
#define WLAN_HDD_IS_NDP_ENABLED(hdd_ctx) (false)
#endif

/* Set mac address locally administered bit */
#define WLAN_HDD_RESET_LOCALLY_ADMINISTERED_BIT(macaddr) (macaddr[0] &= 0xFD)

#define HDD_DEFAULT_MCC_P2P_QUOTA    70
#define HDD_RESET_MCC_P2P_QUOTA      50

/*
 * struct hdd_priv_data - driver ioctl private data payload
 * @buf: pointer to command buffer (may be in userspace)
 * @used_len: length of the command/data currently in @buf
 * @total_len: total length of the @buf memory allocation
 */
struct hdd_priv_data {
	uint8_t *buf;
	int used_len;
	int total_len;
};

#define  MAX_MOD_LOGLEVEL 10
struct fw_log_info {
	uint8_t enable;
	uint8_t dl_type;
	uint8_t dl_report;
	uint8_t dl_loglevel;
	uint8_t index;
	uint32_t dl_mod_loglevel[MAX_MOD_LOGLEVEL];

};

/**
 * enum antenna_mode - number of TX/RX chains
 * @HDD_ANTENNA_MODE_INVALID: Invalid mode place holder
 * @HDD_ANTENNA_MODE_1X1: Number of TX/RX chains equals 1
 * @HDD_ANTENNA_MODE_2X2: Number of TX/RX chains equals 2
 * @HDD_ANTENNA_MODE_MAX: Place holder for max mode
 */
enum antenna_mode {
	HDD_ANTENNA_MODE_INVALID,
	HDD_ANTENNA_MODE_1X1,
	HDD_ANTENNA_MODE_2X2,
	HDD_ANTENNA_MODE_MAX
};

/**
 * enum smps_mode - SM power save mode
 * @HDD_SMPS_MODE_STATIC: Static power save
 * @HDD_SMPS_MODE_DYNAMIC: Dynamic power save
 * @HDD_SMPS_MODE_RESERVED: Reserved
 * @HDD_SMPS_MODE_DISABLED: Disable power save
 * @HDD_SMPS_MODE_MAX: Place holder for max mode
 */
enum smps_mode {
	HDD_SMPS_MODE_STATIC,
	HDD_SMPS_MODE_DYNAMIC,
	HDD_SMPS_MODE_RESERVED,
	HDD_SMPS_MODE_DISABLED,
	HDD_SMPS_MODE_MAX
};

#ifdef WLAN_FEATURE_OFFLOAD_PACKETS
/**
 * struct hdd_offloaded_packets - request id to pattern id mapping
 * @request_id: request id
 * @pattern_id: pattern id
 *
 */
struct hdd_offloaded_packets {
	uint32_t request_id;
	uint8_t  pattern_id;
};

/**
 * struct hdd_offloaded_packets_ctx - offloaded packets context
 * @op_table: request id to pattern id table
 * @op_lock: mutex lock
 */
struct hdd_offloaded_packets_ctx {
	struct hdd_offloaded_packets op_table[MAXNUM_PERIODIC_TX_PTRNS];
	struct mutex op_lock;
};
#endif

/**
 * enum driver_status: Driver Modules status
 * @DRIVER_MODULES_UNINITIALIZED: Driver CDS modules uninitialized
 * @DRIVER_MODULES_ENABLED: Driver CDS modules opened
 * @DRIVER_MODULES_CLOSED: Driver CDS modules closed
 */
enum driver_modules_status {
	DRIVER_MODULES_UNINITIALIZED,
	DRIVER_MODULES_ENABLED,
	DRIVER_MODULES_CLOSED
};

/**
 * struct acs_dfs_policy - Define ACS policies
 * @acs_dfs_mode: Dfs mode enabled/disabled.
 * @acs_channel: pre defined channel to avoid ACS.
 */
struct acs_dfs_policy {
	enum dfs_mode acs_dfs_mode;
	uint8_t acs_channel;
};

/**
 * enum suspend_fail_reason: Reasons a WLAN suspend might fail
 * SUSPEND_FAIL_IPA: IPA in progress
 * SUSPEND_FAIL_RADAR: radar scan in progress
 * SUSPEND_FAIL_ROAM: roaming in progress
 * SUSPEND_FAIL_SCAN: scan in progress
 * SUSPEND_FAIL_INITIAL_WAKEUP: received initial wakeup from firmware
 * SUSPEND_FAIL_MAX_COUNT: the number of wakeup reasons, always at the end
 */
enum suspend_fail_reason {
	SUSPEND_FAIL_IPA,
	SUSPEND_FAIL_RADAR,
	SUSPEND_FAIL_ROAM,
	SUSPEND_FAIL_SCAN,
	SUSPEND_FAIL_INITIAL_WAKEUP,
	SUSPEND_FAIL_MAX_COUNT
};

/**
 * suspend_resume_stats - Collection of counters for suspend/resume events
 * @suspends: number of suspends completed
 * @resumes: number of resumes completed
 * @suspend_fail: counters for failed suspend reasons
 */
struct suspend_resume_stats {
	uint32_t suspends;
	uint32_t resumes;
	uint32_t suspend_fail[SUSPEND_FAIL_MAX_COUNT];
};

/**
 * hdd_sta_smps_param  - SMPS parameters to configure from hdd
 * HDD_STA_SMPS_PARAM_UPPER_RSSI_THRESH: RSSI threshold to enter Dynamic SMPS
 * mode from inactive mode
 * HDD_STA_SMPS_PARAM_STALL_RSSI_THRESH:  RSSI threshold to enter
 * Stalled-D-SMPS mode from D-SMPS mode or to enter D-SMPS mode from
 * Stalled-D-SMPS mode
 * HDD_STA_SMPS_PARAM_LOWER_RSSI_THRESH:  RSSI threshold to disable SMPS modes
 * HDD_STA_SMPS_PARAM_UPPER_BRSSI_THRESH: Upper threshold for beacon-RSSI.
 * Used to reduce RX chainmask.
 * HDD_STA_SMPS_PARAM_LOWER_BRSSI_THRESH:  Lower threshold for beacon-RSSI.
 * Used to increase RX chainmask.
 * HDD_STA_SMPS_PARAM_DTIM_1CHRX_ENABLE: Enable/Disable DTIM 1chRx feature
 */
enum hdd_sta_smps_param {
	HDD_STA_SMPS_PARAM_UPPER_RSSI_THRESH = 0,
	HDD_STA_SMPS_PARAM_STALL_RSSI_THRESH = 1,
	HDD_STA_SMPS_PARAM_LOWER_RSSI_THRESH = 2,
	HDD_STA_SMPS_PARAM_UPPER_BRSSI_THRESH = 3,
	HDD_STA_SMPS_PARAM_LOWER_BRSSI_THRESH = 4,
	HDD_STA_SMPS_PARAM_DTIM_1CHRX_ENABLE = 5
};

/**
 * enum RX_OFFLOAD - Receive offload modes
 * @CFG_LRO_ENABLED: Large Rx offload
 * @CFG_GRO_ENABLED: Generic Rx Offload
 */
enum RX_OFFLOAD {
	CFG_LRO_ENABLED = 1,
	CFG_GRO_ENABLED,
};

/* One per STA: 1 for BCMC_STA_ID, 1 for each SAP_SELF_STA_ID,
 * 1 for WDS_STAID
 */
#define HDD_MAX_ADAPTERS (WLAN_MAX_STA_COUNT + QDF_MAX_NO_OF_SAP_MODE + 2)

#ifdef DISABLE_CHANNEL_LIST

/**
 * struct hdd_cache_channel_info - Structure of the channel info
 * which needs to be cached
 * @channel_num: channel number
 * @reg_status: Current regulatory status of the channel
 * Enable
 * Disable
 * DFS
 * Invalid
 * @wiphy_status: Current wiphy status
 */
struct hdd_cache_channel_info {
	uint32_t channel_num;
	enum channel_state reg_status;
	uint32_t wiphy_status;
};

/**
 * struct hdd_cache_channels - Structure of the channels to be cached
 * @num_channels: Number of channels to be cached
 * @channel_info: Structure of the channel info
 */
struct hdd_cache_channels {
	uint32_t num_channels;
	struct hdd_cache_channel_info *channel_info;
};
#endif

/**
 * struct hdd_dynamic_mac - hdd structure to handle dynamic mac address changes
 * @dynamic_mac: Dynamicaly configured mac, this contains the mac on which
 * current interface is up
 * @is_provisioned_mac: is this mac from provisioned list
 * @bit_position: holds the bit mask position from where this mac is assigned,
 * if mac is assigned from provisioned this field contains the position from
 * provisioned_intf_addr_mask else contains the position from
 * derived_intf_addr_mask
 */
struct hdd_dynamic_mac {
	struct qdf_mac_addr dynamic_mac;
	bool is_provisioned_mac;
	uint8_t bit_position;
};

/**
 * hdd_fw_ver_info - FW version info structure
 * @major_spid: FW version - major spid.
 * @minor_spid: FW version - minor spid
 * @siid:       FW version - siid
 * @sub_id:     FW version - sub id
 * @rel_id:     FW version - release id
 * @crmid:      FW version - crmid
 */

struct hdd_fw_ver_info {
	uint32_t major_spid;
	uint32_t minor_spid;
	uint32_t siid;
	uint32_t sub_id;
	uint32_t rel_id;
	uint32_t crmid;
};

/**
 * struct hdd_context - hdd shared driver and psoc/device context
 * @psoc: object manager psoc context
 * @pdev: object manager pdev context
 * @bus_bw_work: work for periodically computing DDR bus bandwidth requirements
 * @g_event_flags: a bitmap of hdd_driver_flags
 * @psoc_idle_timeout_work: delayed work for psoc idle shutdown
 * @dynamic_nss_chains_support: Per vdev dynamic nss chains update capability
 * @sar_cmd_params: SAR command params to be configured to the FW
 */
struct hdd_context {
	struct wlan_objmgr_psoc *psoc;
	struct wlan_objmgr_pdev *pdev;
	mac_handle_t mac_handle;
	struct wiphy *wiphy;
	qdf_spinlock_t hdd_adapter_lock;
	qdf_list_t hdd_adapters; /* List of adapters */

	struct hdd_adapter *sta_to_adapter[HDD_MAX_ADAPTERS];

	/** Pointer for firmware image data */
	const struct firmware *fw;

	/** Pointer for configuration data */
	const struct firmware *cfg;

	/** Pointer to the parent device */
	struct device *parent_dev;

	/** Config values read from qcom_cfg.ini file */
	struct hdd_config *config;

	/* Pointer for wiphy 2G/5G band channels */
	struct ieee80211_channel *channels_2ghz;
	struct ieee80211_channel *channels_5ghz;

	/* Completion  variable to indicate Mc Thread Suspended */
	struct completion mc_sus_event_var;

	bool is_scheduler_suspended;

#ifdef QCA_CONFIG_SMP
	bool is_ol_rx_thread_suspended;
#endif

	bool hdd_wlan_suspended;
	bool suspended;
	/* flag to start pktlog after SSR/PDR if previously enabled */
	bool is_pktlog_enabled;

	/* Lock to avoid race condition during start/stop bss */
	struct mutex sap_lock;

#ifdef FEATURE_OEM_DATA_SUPPORT
	/* OEM App registered or not */
	bool oem_app_registered;

	/* OEM App Process ID */
	int32_t oem_pid;
#endif

	/** Concurrency Parameters*/
	uint32_t concurrency_mode;

	uint8_t no_of_open_sessions[QDF_MAX_NO_OF_MODE];
	uint8_t no_of_active_sessions[QDF_MAX_NO_OF_MODE];

	/** P2P Device MAC Address for the adapter  */
	struct qdf_mac_addr p2p_device_address;

	qdf_wake_lock_t rx_wake_lock;
	qdf_wake_lock_t sap_wake_lock;

	/* Flag keeps track of wiphy suspend/resume */
	bool is_wiphy_suspended;

#ifdef WLAN_FEATURE_DP_BUS_BANDWIDTH
	struct qdf_periodic_work bus_bw_work;
	int cur_vote_level;
	spinlock_t bus_bw_lock;
	int cur_rx_level;
	uint64_t prev_no_rx_offload_pkts;
	uint64_t prev_rx_offload_pkts;
	uint64_t prev_no_tx_offload_pkts;
	uint64_t prev_tx_offload_pkts;
	int cur_tx_level;
	uint64_t prev_tx;
#endif /*WLAN_FEATURE_DP_BUS_BANDWIDTH*/

	struct completion ready_to_suspend;
	/* defining the solution type */
	uint32_t target_type;

	/* defining the firmware version */
	uint32_t target_fw_version;
	uint32_t target_fw_vers_ext;
	struct hdd_fw_ver_info fw_version_info;

	/* defining the chip/rom version */
	uint32_t target_hw_version;
	/* defining the chip/rom revision */
	uint32_t target_hw_revision;
	/* chip/rom name */
	char *target_hw_name;
	struct regulatory reg;
#ifdef FEATURE_WLAN_CH_AVOID
	uint16_t unsafe_channel_count;
	uint16_t unsafe_channel_list[NUM_CHANNELS];
#endif /* FEATURE_WLAN_CH_AVOID */

	uint8_t max_intf_count;
	uint8_t current_intf_count;
#ifdef WLAN_FEATURE_LPSS
	uint8_t lpss_support;
#endif
	uint8_t ap_arpns_support;
	tSirScanType ioctl_scan_mode;

#ifdef FEATURE_WLAN_MCC_TO_SCC_SWITCH
	qdf_work_t sta_ap_intf_check_work;
#endif

	uint8_t dev_dfs_cac_status;

	bool bt_coex_mode_set;
#ifdef FEATURE_WLAN_AP_AP_ACS_OPTIMIZE
	qdf_mc_timer_t skip_acs_scan_timer;
	uint8_t skip_acs_scan_status;
	uint8_t *last_acs_channel_list;
	uint8_t num_of_channels;
	qdf_spinlock_t acs_skip_lock;
#endif

	qdf_wake_lock_t sap_dfs_wakelock;
	atomic_t sap_dfs_ref_cnt;

#ifdef WLAN_FEATURE_EXTWOW_SUPPORT
	bool is_extwow_app_type1_param_set;
	bool is_extwow_app_type2_param_set;
#endif

	/* Time since boot up to extscan start (in micro seconds) */
	uint64_t ext_scan_start_since_boot;
	unsigned long g_event_flags;
	uint8_t miracast_value;

#ifdef WLAN_NS_OFFLOAD
	/* IPv6 notifier callback for handling NS offload on change in IP */
	struct notifier_block ipv6_notifier;
#endif
	bool ns_offload_enable;
	/* IPv4 notifier callback for handling ARP offload on change in IP */
	struct notifier_block ipv4_notifier;

	/* number of rf chains supported by target */
	uint32_t  num_rf_chains;
	/* Is htTxSTBC supported by target */
	uint8_t   ht_tx_stbc_supported;
#ifdef WLAN_FEATURE_OFFLOAD_PACKETS
	struct hdd_offloaded_packets_ctx op_ctx;
#endif
	bool mcc_mode;
	struct mutex memdump_lock;
	uint16_t driver_dump_size;
	uint8_t *driver_dump_mem;

	bool connection_in_progress;
	qdf_spinlock_t connection_status_lock;

	uint16_t hdd_txrx_hist_idx;
	struct hdd_tx_rx_histogram *hdd_txrx_hist;

	/*
	 * place to store FTM capab of target. This allows changing of FTM capab
	 * at runtime and intersecting it with target capab before updating.
	 */
	uint32_t fine_time_meas_cap_target;
	uint32_t rx_high_ind_cnt;
	/* For Rx thread non GRO/LRO packet accounting */
	uint64_t no_rx_offload_pkt_cnt;
	uint64_t no_tx_offload_pkt_cnt;
	/* Current number of TX X RX chains being used */
	enum antenna_mode current_antenna_mode;

	/* the radio index assigned by cnss_logger */
	int radio_index;
	qdf_work_t sap_pre_cac_work;
	bool hbw_requested;
	enum RX_OFFLOAD ol_enable;
#ifdef WLAN_FEATURE_NAN
	bool nan_datapath_enabled;
#endif
	/* Present state of driver cds modules */
	enum driver_modules_status driver_status;
	struct qdf_delayed_work psoc_idle_timeout_work;
	bool rps;
	bool dynamic_rps;
	bool enable_rxthread;
	/* support for DP RX threads */
	bool enable_dp_rx_threads;
	bool napi_enable;
	struct acs_dfs_policy acs_policy;
	uint16_t wmi_max_len;
	struct suspend_resume_stats suspend_resume_stats;
	struct hdd_runtime_pm_context runtime_context;
	bool roaming_in_progress;
	struct scan_chan_info *chan_info;
	struct mutex chan_info_lock;
	/* bit map to set/reset TDLS by different sources */
	unsigned long tdls_source_bitmap;
	bool tdls_umac_comp_active;
	bool tdls_nap_active;
	uint8_t beacon_probe_rsp_cnt_per_scan;
	uint8_t last_scan_reject_vdev_id;
	enum scan_reject_states last_scan_reject_reason;
	unsigned long last_scan_reject_timestamp;
	uint8_t scan_reject_cnt;
	bool dfs_cac_offload;
	bool reg_offload;
	bool rcpi_enabled;
#ifdef FEATURE_WLAN_CH_AVOID
	struct ch_avoid_ind_type coex_avoid_freq_list;
	struct ch_avoid_ind_type dnbs_avoid_freq_list;
	/* Lock to control access to dnbs and coex avoid freq list */
	struct mutex avoid_freq_lock;
#endif
#ifdef WLAN_FEATURE_TSF
	/* indicate whether tsf has been initialized */
	qdf_atomic_t tsf_ready_flag;
	/* indicate whether it's now capturing tsf(updating tstamp-pair) */
	qdf_atomic_t cap_tsf_flag;
	/* the context that is capturing tsf */
	struct hdd_adapter *cap_tsf_context;
#endif
#ifdef WLAN_FEATURE_TSF_PTP
	struct ptp_clock_info ptp_cinfo;
	struct ptp_clock *ptp_clock;
#endif
	uint8_t bt_a2dp_active:1;
	uint8_t bt_vo_active:1;
	enum band_info curr_band;
	bool imps_enabled;
#ifdef WLAN_FEATURE_PACKET_FILTERING
	int user_configured_pkt_filter_rules;
#endif
	bool is_fils_roaming_supported;
	QDF_STATUS (*receive_offload_cb)(struct hdd_adapter *,
					 struct sk_buff *);
	qdf_atomic_t vendor_disable_lro_flag;

	/* disable RX offload (GRO/LRO) in concurrency scenarios */
	qdf_atomic_t disable_rx_ol_in_concurrency;
	/* disable RX offload (GRO/LRO) in low throughput scenarios */
	qdf_atomic_t disable_rx_ol_in_low_tput;
	bool en_tcp_delack_no_lro;
	bool force_rsne_override;
	qdf_wake_lock_t monitor_mode_wakelock;
	bool lte_coex_ant_share;
	bool obss_scan_offload;
	int sscan_pid;
	uint32_t track_arp_ip;

	/* defining the board related information */
	uint32_t hw_bd_id;
	struct board_info hw_bd_info;
#ifdef WLAN_SUPPORT_TWT
	enum twt_status twt_state;
#endif
#ifdef FEATURE_WLAN_APF
	uint32_t apf_version;
	bool apf_enabled_v2;
#endif

#ifdef DISABLE_CHANNEL_LIST
	struct hdd_cache_channels *original_channels;
	qdf_mutex_t cache_channel_lock;
#endif
	enum sar_version sar_version;
	struct hdd_dynamic_mac dynamic_mac_list[QDF_MAX_CONCURRENCY_PERSONA];
	bool dynamic_nss_chains_support;
	struct qdf_mac_addr hw_macaddr;
	struct qdf_mac_addr provisioned_mac_addr[QDF_MAX_CONCURRENCY_PERSONA];
	struct qdf_mac_addr derived_mac_addr[QDF_MAX_CONCURRENCY_PERSONA];
	uint32_t num_provisioned_addr;
	uint32_t num_derived_addr;
	unsigned long provisioned_intf_addr_mask;
	unsigned long derived_intf_addr_mask;

	struct sar_limit_cmd_params *sar_cmd_params;

#ifdef CLD_PM_QOS
	struct pm_qos_request pm_qos_req;
#endif
};

/**
 * struct hdd_vendor_acs_chan_params - vendor acs channel parameters
 * @channel_count: channel count
 * @channel_list: pointer to channel list
 * @pcl_count: pcl list count
 * @vendor_pcl_list: pointer to pcl list
 * @vendor_weight_list: pointer to pcl weight list
 */
struct hdd_vendor_acs_chan_params {
	uint32_t channel_count;
	uint8_t *channel_list;
	uint32_t pcl_count;
	uint8_t *vendor_pcl_list;
	uint8_t *vendor_weight_list;
};

/**
 * struct hdd_external_acs_timer_context - acs timer context
 * @reason: reason for acs trigger
 * @adapter: hdd adapter for acs
 */
struct hdd_external_acs_timer_context {
	int8_t reason;
	struct hdd_adapter *adapter;
};

/**
 * struct hdd_vendor_chan_info - vendor channel info
 * @band: channel operating band
 * @pri_ch: primary channel
 * @ht_sec_ch: secondary channel
 * @vht_seg0_center_ch: segment0 for vht
 * @vht_seg1_center_ch: vht segment 1
 * @chan_width: channel width
 */
struct hdd_vendor_chan_info {
	uint8_t band;
	uint8_t pri_ch;
	uint8_t ht_sec_ch;
	uint8_t vht_seg0_center_ch;
	uint8_t vht_seg1_center_ch;
	uint8_t chan_width;
};

/**
 * struct  hdd_channel_info - standard channel info
 * @freq: Freq in Mhz
 * @flags: channel info flags
 * @flagext: extended channel info flags
 * @ieee_chan_number: channel number
 * @max_reg_power: max tx power according to regulatory
 * @max_radio_power: max radio power
 * @min_radio_power: min radio power
 * @reg_class_id: regulatory class
 * @max_antenna_gain: max antenna gain allowed on channel
 * @vht_center_freq_seg0: vht center freq segment 0
 * @vht_center_freq_seg1: vht center freq segment 1
 */
struct hdd_channel_info {
	u_int16_t freq;
	u_int32_t flags;
	u_int16_t flagext;
	u_int8_t ieee_chan_number;
	int8_t max_reg_power;
	int8_t max_radio_power;
	int8_t min_radio_power;
	u_int8_t reg_class_id;
	u_int8_t max_antenna_gain;
	u_int8_t vht_center_freq_seg0;
	u_int8_t vht_center_freq_seg1;
};

/*
 * Function declarations and documentation
 */
#ifdef SEC_CONFIG_PSM_SYSFS
int wlan_hdd_sec_get_psm(void);
#endif /* SEC_CONFIG_PSM_SYSFS */
int hdd_validate_channel_and_bandwidth(struct hdd_adapter *adapter,
				uint32_t chan_number,
				enum phy_ch_width chan_bw);

QDF_STATUS hdd_get_front_adapter(struct hdd_context *hdd_ctx,
				 struct hdd_adapter **out_adapter);

QDF_STATUS hdd_get_next_adapter(struct hdd_context *hdd_ctx,
				struct hdd_adapter *current_adapter,
				struct hdd_adapter **out_adapter);

QDF_STATUS hdd_remove_adapter(struct hdd_context *hdd_ctx,
			      struct hdd_adapter *adapter);

QDF_STATUS hdd_remove_front_adapter(struct hdd_context *hdd_ctx,
				    struct hdd_adapter **out_adapter);

QDF_STATUS hdd_add_adapter_back(struct hdd_context *hdd_ctx,
				struct hdd_adapter *adapter);

QDF_STATUS hdd_add_adapter_front(struct hdd_context *hdd_ctx,
				 struct hdd_adapter *adapter);

/**
 * hdd_for_each_adapter - adapter iterator macro
 * @hdd_ctx: the global HDD context
 * @adapter: an hdd_adapter pointer to use as a cursor
 */
#define hdd_for_each_adapter(hdd_ctx, adapter) \
	for (hdd_get_front_adapter(hdd_ctx, &adapter); \
	     adapter; \
	     hdd_get_next_adapter(hdd_ctx, adapter, &adapter))

struct hdd_adapter *hdd_open_adapter(struct hdd_context *hdd_ctx,
				     uint8_t session_type,
				     const char *name, tSirMacAddr mac_addr,
				     unsigned char name_assign_type,
				     bool rtnl_held);

/**
 * hdd_close_adapter() - remove and free @adapter from the adapter list
 * @hdd_ctx: The Hdd context containing the adapter list
 * @adapter: the adapter to remove and free
 * @rtnl_held: if the caller is already holding the RTNL lock
 *
 * Return: None
 */
void hdd_close_adapter(struct hdd_context *hdd_ctx,
		       struct hdd_adapter *adapter,
		       bool rtnl_held);

/**
 * hdd_close_all_adapters() - remove and free all adapters from the adapter list
 * @hdd_ctx: The Hdd context containing the adapter list
 * @rtnl_held: if the caller is already holding the RTNL lock
 *
 * Return: None
 */
void hdd_close_all_adapters(struct hdd_context *hdd_ctx, bool rtnl_held);

QDF_STATUS hdd_stop_all_adapters(struct hdd_context *hdd_ctx);
void hdd_deinit_all_adapters(struct hdd_context *hdd_ctx, bool rtnl_held);
QDF_STATUS hdd_reset_all_adapters(struct hdd_context *hdd_ctx);
QDF_STATUS hdd_start_all_adapters(struct hdd_context *hdd_ctx);

/**
 * hdd_get_adapter_by_vdev() - Return adapter with the given vdev id
 * @hdd_ctx: hdd context.
 * @vdev_id: vdev id for the adapter to get.
 *
 * This function is used to get the adapter with provided vdev id
 *
 * Return: adapter pointer if found
 *
 */
struct hdd_adapter *hdd_get_adapter_by_vdev(struct hdd_context *hdd_ctx,
					    uint32_t vdev_id);

struct hdd_adapter *hdd_get_adapter_by_macaddr(struct hdd_context *hdd_ctx,
					       tSirMacAddr mac_addr);

/**
 * hdd_get_adapter_home_channel() - return home channel of adapter
 * @adapter: hdd adapter of vdev
 *
 * This function returns operation channel of station/p2p-cli if
 * connected, returns opration channel of sap/p2p-go if started.
 *
 * Return: home channel if connected/started or invalid channel 0
 */
uint8_t hdd_get_adapter_home_channel(struct hdd_adapter *adapter);

/*
 * hdd_get_adapter_by_rand_macaddr() - find Random mac adapter
 * @hdd_ctx: hdd context
 * @mac_addr: random mac addr
 *
 * Find the Adapter based on random mac addr. Adapter's vdev
 * have active random mac list.
 *
 * Return: adapter ptr or null
 */
struct hdd_adapter *
hdd_get_adapter_by_rand_macaddr(struct hdd_context *hdd_ctx,
				tSirMacAddr mac_addr);

/**
 * hdd_is_vdev_in_conn_state() - Check whether the vdev is in
 * connected/started state.
 * @adapter: hdd adapter of the vdev
 *
 * This function will give whether the vdev in the adapter is in
 * connected/started state.
 *
 * Return: True/false
 */
bool hdd_is_vdev_in_conn_state(struct hdd_adapter *adapter);

int hdd_vdev_create(struct hdd_adapter *adapter,
		    csr_roam_complete_cb callback, void *ctx);
int hdd_vdev_destroy(struct hdd_adapter *adapter);
int hdd_vdev_ready(struct hdd_adapter *adapter);

QDF_STATUS hdd_init_station_mode(struct hdd_adapter *adapter);
struct hdd_adapter *hdd_get_adapter(struct hdd_context *hdd_ctx,
			enum QDF_OPMODE mode);

/**
 * hdd_get_device_mode() - Get device mode
 * @vdev_id: vdev id
 *
 * Return: Device mode
 */
enum QDF_OPMODE hdd_get_device_mode(uint32_t vdev_id);

void hdd_deinit_adapter(struct hdd_context *hdd_ctx,
			struct hdd_adapter *adapter,
			bool rtnl_held);
QDF_STATUS hdd_stop_adapter(struct hdd_context *hdd_ctx,
			    struct hdd_adapter *adapter);

void hdd_set_station_ops(struct net_device *dev);

/**
 * wlan_hdd_get_intf_addr() - Get address for the interface
 * @hdd_ctx: Pointer to hdd context
 * @interface_type: type of the interface for which address is queried
 *
 * This function is used to get mac address for every new interface
 *
 * Return: If addr is present then return pointer to MAC address
 *         else NULL
 */

uint8_t *wlan_hdd_get_intf_addr(struct hdd_context *hdd_ctx,
				enum QDF_OPMODE interface_type);
void wlan_hdd_release_intf_addr(struct hdd_context *hdd_ctx,
				uint8_t *releaseAddr);
uint8_t hdd_get_operating_channel(struct hdd_context *hdd_ctx,
			enum QDF_OPMODE mode);

void hdd_set_conparam(int32_t con_param);
enum QDF_GLOBAL_MODE hdd_get_conparam(void);
void wlan_hdd_reset_prob_rspies(struct hdd_adapter *adapter);
void hdd_prevent_suspend(uint32_t reason);

/*
 * hdd_get_first_valid_adapter() - Get the first valid adapter from adapter list
 *
 * This function is used to fetch the first valid adapter from the adapter
 * list. If there is no valid adapter then it returns NULL
 *
 * @hdd_ctx: HDD context handler
 *
 * Return: NULL if no valid adapter found in the adapter list
 *
 */
struct hdd_adapter *hdd_get_first_valid_adapter(struct hdd_context *hdd_ctx);

void hdd_allow_suspend(uint32_t reason);
void hdd_prevent_suspend_timeout(uint32_t timeout, uint32_t reason);

QDF_STATUS hdd_set_ibss_power_save_params(struct hdd_adapter *adapter);

/**
 * wlan_hdd_validate_context() - check the HDD context
 * @hdd_ctx: Global HDD context pointer
 *
 * Return: 0 if the context is valid. Error code otherwise
 */
#define wlan_hdd_validate_context(hdd_ctx) \
	__wlan_hdd_validate_context(hdd_ctx, __func__)

int __wlan_hdd_validate_context(struct hdd_context *hdd_ctx, const char *func);

/**
 * hdd_validate_adapter() - Validate the given adapter
 * @adapter: the adapter to validate
 *
 * This function validates the given adapter, and ensures that it is open.
 *
 * Return: Errno
 */
#define hdd_validate_adapter(adapter) \
	__hdd_validate_adapter(adapter, __func__)

int __hdd_validate_adapter(struct hdd_adapter *adapter, const char *func);

/**
 * wlan_hdd_validate_vdev_id() - ensure the given vdev Id is valid
 * @vdev_id: the vdev Id to validate
 *
 * Return: Errno
 */
#define wlan_hdd_validate_vdev_id(vdev_id) \
	__wlan_hdd_validate_vdev_id(vdev_id, __func__)

int __wlan_hdd_validate_vdev_id(uint8_t vdev_id, const char *func);

/**
 * hdd_is_valid_mac_address() - validate MAC address
 * @mac_addr:	Pointer to the input MAC address
 *
 * This function validates whether the given MAC address is valid or not
 * Expected MAC address is of the format XX:XX:XX:XX:XX:XX
 * where X is the hexa decimal digit character and separated by ':'
 * This algorithm works even if MAC address is not separated by ':'
 *
 * This code checks given input string mac contains exactly 12 hexadecimal
 * digits and a separator colon : appears in the input string only after
 * an even number of hex digits.
 *
 * Return: true for valid and false for invalid
 */
bool hdd_is_valid_mac_address(const uint8_t *mac_addr);

bool wlan_hdd_validate_modules_state(struct hdd_context *hdd_ctx);

/**
 * wlan_hdd_validate_mac_address() - Function to validate mac address
 * @mac_addr: input mac address
 *
 * Return QDF_STATUS
 */
#define wlan_hdd_validate_mac_address(mac_addr) \
	__wlan_hdd_validate_mac_address(mac_addr, __func__)

QDF_STATUS __wlan_hdd_validate_mac_address(struct qdf_mac_addr *mac_addr,
					   const char *func);

#ifdef WLAN_FEATURE_DP_BUS_BANDWIDTH
/**
 * hdd_bus_bw_compute_timer_start() - start the bandwidth timer
 * @hdd_ctx: the global hdd context
 *
 * Return: None
 */
void hdd_bus_bw_compute_timer_start(struct hdd_context *hdd_ctx);

/**
 * hdd_bus_bw_compute_timer_try_start() - try to start the bandwidth timer
 * @hdd_ctx: the global hdd context
 *
 * This function ensures there is at least one adapter in the associated state
 * before starting the bandwidth timer.
 *
 * Return: None
 */
void hdd_bus_bw_compute_timer_try_start(struct hdd_context *hdd_ctx);

/**
 * hdd_bus_bw_compute_timer_stop() - stop the bandwidth timer
 * @hdd_ctx: the global hdd context
 *
 * Return: None
 */
void hdd_bus_bw_compute_timer_stop(struct hdd_context *hdd_ctx);

/**
 * hdd_bus_bw_compute_timer_try_stop() - try to stop the bandwidth timer
 * @hdd_ctx: the global hdd context
 *
 * This function ensures there are no adapters in the associated state before
 * stopping the bandwidth timer.
 *
 * Return: None
 */
void hdd_bus_bw_compute_timer_try_stop(struct hdd_context *hdd_ctx);

/**
 * hdd_bus_bandwidth_init() - Initialize bus bandwidth data structures.
 * @hdd_ctx: HDD context
 *
 * Initialize bus bandwidth related data structures like spinlock and timer.
 *
 * Return: None.
 */
int hdd_bus_bandwidth_init(struct hdd_context *hdd_ctx);

/**
 * hdd_bus_bandwidth_deinit() - De-initialize bus bandwidth data structures.
 * @hdd_ctx: HDD context
 *
 * De-initialize bus bandwidth related data structures like timer.
 *
 * Return: None.
 */
void hdd_bus_bandwidth_deinit(struct hdd_context *hdd_ctx);

#define GET_CUR_RX_LVL(config) ((config)->cur_rx_level)
#define GET_BW_COMPUTE_INTV(config) ((config)->bus_bw_compute_interval)
#else

static inline
void hdd_bus_bw_compute_timer_start(struct hdd_context *hdd_ctx)
{
}

static inline
void hdd_bus_bw_compute_timer_try_start(struct hdd_context *hdd_ctx)
{
}

static inline
void hdd_bus_bw_compute_timer_stop(struct hdd_context *hdd_ctx)
{
}

static inline
void hdd_bus_bw_compute_timer_try_stop(struct hdd_context *hdd_ctx)
{
}

static inline
int hdd_bus_bandwidth_init(struct hdd_context *hdd_ctx)
{
	return 0;
}

static inline
void hdd_bus_bandwidth_deinit(struct hdd_context *hdd_ctx)
{
}

#define GET_CUR_RX_LVL(config) 0
#define GET_BW_COMPUTE_INTV(config) 0

#endif /*WLAN_FEATURE_DP_BUS_BANDWIDTH*/

int hdd_qdf_trace_enable(QDF_MODULE_ID module_id, uint32_t bitmask);

int hdd_init(void);
void hdd_deinit(void);

/**
 * hdd_wlan_startup() - HDD init function
 * hdd_ctx: the HDD context corresponding to the psoc to startup
 *
 * Return: Errno
 */
int hdd_wlan_startup(struct hdd_context *hdd_ctx);

/**
 * hdd_wlan_exit() - HDD WLAN exit function
 * @hdd_ctx: pointer to the HDD Context
 *
 * Return: None
 */
void hdd_wlan_exit(struct hdd_context *hdd_ctx);

/**
 * hdd_psoc_create_vdevs() - create the default vdevs for a psoc
 * @hdd_ctx: the HDD context for the psoc to operate against
 *
 * Return: QDF_STATUS
 */
QDF_STATUS hdd_psoc_create_vdevs(struct hdd_context *hdd_ctx);

/*
 * hdd_context_create() - Allocate and inialize HDD context.
 * @dev: Device Pointer to the underlying device
 *
 * Allocate and initialize HDD context. HDD context is allocated as part of
 * wiphy allocation and then context is initialized.
 *
 * Return: HDD context on success and ERR_PTR on failure
 */
struct hdd_context *hdd_context_create(struct device *dev);

/**
 * hdd_context_destroy() - Destroy HDD context
 * @hdd_ctx: HDD context to be destroyed.
 *
 * Free config and HDD context as well as destroy all the resources.
 *
 * Return: None
 */
void hdd_context_destroy(struct hdd_context *hdd_ctx);

int hdd_wlan_notify_modem_power_state(int state);

void wlan_hdd_send_svc_nlink_msg(int radio, int type, void *data, int len);
#ifdef FEATURE_WLAN_AUTO_SHUTDOWN
void wlan_hdd_auto_shutdown_enable(struct hdd_context *hdd_ctx, bool enable);
#endif

struct hdd_adapter *
hdd_get_con_sap_adapter(struct hdd_adapter *this_sap_adapter,
			bool check_start_bss);

bool hdd_is_5g_supported(struct hdd_context *hdd_ctx);

/**
 * hdd_is_2g_supported() - check if 2GHz channels are supported
 * @hdd_ctx:	Pointer to the hdd context
 *
 * HDD function to know if 2GHz channels are supported
 *
 * Return:  true if 2GHz channels are supported
 */

bool hdd_is_2g_supported(struct hdd_context *hdd_ctx);

int wlan_hdd_scan_abort(struct hdd_adapter *adapter);

#ifdef WLAN_FEATURE_ROAM_OFFLOAD
static inline bool roaming_offload_enabled(struct hdd_context *hdd_ctx)
{
	bool is_roam_offload;

	ucfg_mlme_get_roaming_offload(hdd_ctx->psoc, &is_roam_offload);

	return is_roam_offload;
}
#else
static inline bool roaming_offload_enabled(struct hdd_context *hdd_ctx)
{
	return false;
}
#endif

#ifdef WLAN_FEATURE_HOST_ROAM
static inline bool hdd_driver_roaming_supported(struct hdd_context *hdd_ctx)
{
	bool lfr_enabled;

	ucfg_mlme_is_lfr_enabled(hdd_ctx->psoc, &lfr_enabled);

	return lfr_enabled;
}
#else
static inline bool hdd_driver_roaming_supported(struct hdd_context *hdd_ctx)
{
	return false;
}
#endif

static inline bool hdd_roaming_supported(struct hdd_context *hdd_ctx)
{
	bool val;

	val = hdd_driver_roaming_supported(hdd_ctx) ||
		roaming_offload_enabled(hdd_ctx);

	return val;
}

#ifdef CFG80211_SCAN_RANDOM_MAC_ADDR
static inline bool hdd_scan_random_mac_addr_supported(void)
{
	return true;
}
#else
static inline bool hdd_scan_random_mac_addr_supported(void)
{
	return false;
}
#endif

/**
 * hdd_start_vendor_acs(): Start vendor ACS procedure
 * @adapter: pointer to SAP adapter struct
 *
 * This function sends the ACS config to the ACS daemon and
 * starts the vendor ACS timer to wait for the next command.
 *
 * Return: Status of vendor ACS procedure
 */
int hdd_start_vendor_acs(struct hdd_adapter *adapter);

/**
 * hdd_acs_response_timeout_handler() - timeout handler for acs_timer
 * @context: timeout handler context
 *
 * Return: None
 */
void hdd_acs_response_timeout_handler(void *context);

/**
 * wlan_hdd_cfg80211_start_acs(): Start ACS Procedure for SAP
 * @adapter: pointer to SAP adapter struct
 *
 * This function starts the ACS procedure if there are no
 * constraints like MBSSID DFS restrictions.
 *
 * Return: Status of ACS Start procedure
 */
int wlan_hdd_cfg80211_start_acs(struct hdd_adapter *adapter);

/**
 * hdd_cfg80211_update_acs_config() - update acs config to application
 * @adapter: hdd adapter
 * @reason: channel change reason
 *
 * Return: 0 for success else error code
 */
int hdd_cfg80211_update_acs_config(struct hdd_adapter *adapter,
				   uint8_t reason);

/**
 * hdd_update_acs_timer_reason() - update acs timer start reason
 * @adapter: hdd adapter
 * @reason: channel change reason
 *
 * Return: 0 for success
 */
int hdd_update_acs_timer_reason(struct hdd_adapter *adapter, uint8_t reason);

/**
 * hdd_switch_sap_channel() - Move SAP to the given channel
 * @adapter: AP adapter
 * @channel: Channel
 * @forced: Force to switch channel, ignore SCC/MCC check
 *
 * Moves the SAP interface by invoking the function which
 * executes the callback to perform channel switch using (E)CSA.
 *
 * Return: None
 */
void hdd_switch_sap_channel(struct hdd_adapter *adapter, uint8_t channel,
			    bool forced);

#if defined(FEATURE_WLAN_CH_AVOID)
void hdd_unsafe_channel_restart_sap(struct hdd_context *hdd_ctx);

void hdd_ch_avoid_ind(struct hdd_context *hdd_ctxt,
		      struct unsafe_ch_list *unsafe_chan_list,
		      struct ch_avoid_ind_type *avoid_freq_list);
#else
static inline
void hdd_unsafe_channel_restart_sap(struct hdd_context *hdd_ctx)
{
}

static inline
void hdd_ch_avoid_ind(struct hdd_context *hdd_ctxt,
		      struct unsafe_ch_list *unsafe_chan_list,
		      struct ch_avoid_ind_type *avoid_freq_list)
{
}
#endif

/**
 * hdd_free_mac_address_lists() - Free both the MAC address lists
 * @hdd_ctx: HDD context
 *
 * This API clears/memset provisioned address list and
 * derived address list
 *
 */
void hdd_free_mac_address_lists(struct hdd_context *hdd_ctx);

/**
 * hdd_update_macaddr() - update mac address
 * @hdd_ctx:	hdd contxt
 * @hw_macaddr:	mac address
 * @generate_mac_auto: Indicates whether the first address is
 * provisioned address or derived address.
 *
 * Mac address for multiple virtual interface is found as following
 * i) The mac address of the first interface is just the actual hw mac address.
 * ii) MSM 3 or 4 bits of byte5 of the actual mac address are used to
 *     define the mac address for the remaining interfaces and locally
 *     admistered bit is set. INTF_MACADDR_MASK is based on the number of
 *     supported virtual interfaces, right now this is 0x07 (meaning 8
 *     interface).
 *     Byte[3] of second interface will be hw_macaddr[3](bit5..7) + 1,
 *     for third interface it will be hw_macaddr[3](bit5..7) + 2, etc.
 *
 * Return: None
 */
void hdd_update_macaddr(struct hdd_context *hdd_ctx,
			struct qdf_mac_addr hw_macaddr, bool generate_mac_auto);

/**
 * hdd_store_nss_chains_cfg_in_vdev() - Store the per vdev ini cfg in vdev_obj
 * @adapter: Current HDD adapter passed from caller
 *
 * This function will store the per vdev nss params to the particular mlme
 * vdev obj.
 *
 * Return: None
 */
void
hdd_store_nss_chains_cfg_in_vdev(struct hdd_adapter *adapter);

/**
 * wlan_hdd_disable_roaming() - disable roaming on all STAs except the input one
 * @cur_adapter: Current HDD adapter passed from caller
 *
 * This function loops through all adapters and disables roaming on each STA
 * mode adapter except the current adapter passed from the caller
 *
 * Return: None
 */
void wlan_hdd_disable_roaming(struct hdd_adapter *cur_adapter);

/**
 * wlan_hdd_enable_roaming() - enable roaming on all STAs except the input one
 * @cur_adapter: Current HDD adapter passed from caller
 *
 * This function loops through all adapters and enables roaming on each STA
 * mode adapter except the current adapter passed from the caller
 *
 * Return: None
 */
void wlan_hdd_enable_roaming(struct hdd_adapter *cur_adapter);

QDF_STATUS hdd_post_cds_enable_config(struct hdd_context *hdd_ctx);

QDF_STATUS hdd_abort_mac_scan_all_adapters(struct hdd_context *hdd_ctx);

void wlan_hdd_stop_sap(struct hdd_adapter *ap_adapter);
void wlan_hdd_start_sap(struct hdd_adapter *ap_adapter, bool reinit);

#ifdef QCA_CONFIG_SMP
int wlan_hdd_get_cpu(void);
#else
static inline int wlan_hdd_get_cpu(void)
{
	return 0;
}
#endif

void wlan_hdd_sap_pre_cac_failure(void *data);
void hdd_clean_up_pre_cac_interface(struct hdd_context *hdd_ctx);

void wlan_hdd_txrx_pause_cb(uint8_t vdev_id,
	enum netif_action_type action, enum netif_reason_type reason);

#ifdef QCA_HL_NETDEV_FLOW_CONTROL
void wlan_hdd_mod_fc_timer(struct hdd_adapter *adapter,
			   enum netif_action_type action);
#else
static inline void wlan_hdd_mod_fc_timer(struct hdd_adapter *adapter,
					 enum netif_action_type action)
{
}
#endif /* QCA_HL_NETDEV_FLOW_CONTROL */

int hdd_wlan_dump_stats(struct hdd_adapter *adapter, int value);
void wlan_hdd_deinit_tx_rx_histogram(struct hdd_context *hdd_ctx);
void wlan_hdd_display_tx_rx_histogram(struct hdd_context *hdd_ctx);
void wlan_hdd_clear_tx_rx_histogram(struct hdd_context *hdd_ctx);

void
wlan_hdd_display_netif_queue_history(struct hdd_context *hdd_ctx,
				     enum qdf_stats_verbosity_level verb_lvl);
void wlan_hdd_clear_netif_queue_history(struct hdd_context *hdd_ctx);
const char *hdd_get_fwpath(void);
void hdd_indicate_mgmt_frame(tSirSmeMgmtFrameInd *frame_ind);

/**
 * hdd_get_adapter_by_iface_name() - Return adapter with given interface name
 * @hdd_ctx: hdd context.
 * @iface_name: interface name
 *
 * This function is used to get the adapter with given interface name
 *
 * Return: adapter pointer if found, NULL otherwise
 *
 */
struct hdd_adapter *hdd_get_adapter_by_iface_name(struct hdd_context *hdd_ctx,
					     const char *iface_name);
enum phy_ch_width hdd_map_nl_chan_width(enum nl80211_chan_width ch_width);

/**
 * hdd_nl_to_qdf_iface_type() - map nl80211_iftype to QDF_OPMODE
 * @nl_type: the input NL80211 interface type to map
 * @out_qdf_type: the output, equivalent QDF operating mode
 *
 * Return: QDF_STATUS
 */
QDF_STATUS hdd_nl_to_qdf_iface_type(enum nl80211_iftype nl_type,
				    enum QDF_OPMODE *out_qdf_type);

/**
 * wlan_hdd_find_opclass() - Find operating class for a channel
 * @mac_handle: global MAC handle
 * @channel: channel id
 * @bw_offset: bandwidth offset
 *
 * Function invokes sme api to find the operating class
 *
 * Return: operating class
 */
uint8_t wlan_hdd_find_opclass(mac_handle_t mac_handle, uint8_t channel,
			      uint8_t bw_offset);

int hdd_update_config(struct hdd_context *hdd_ctx);

/**
 * hdd_update_components_config() - Initialize driver per module ini parameters
 * @hdd_ctx: HDD Context
 *
 * API is used to initialize components configuration parameters
 * Return: 0 for success, errno for failure
 */
int hdd_update_components_config(struct hdd_context *hdd_ctx);

QDF_STATUS hdd_chan_change_notify(struct hdd_adapter *adapter,
		struct net_device *dev,
		struct hdd_chan_change_params chan_change,
		bool legacy_phymode);
int wlan_hdd_set_channel(struct wiphy *wiphy,
		struct net_device *dev,
		struct cfg80211_chan_def *chandef,
		enum nl80211_channel_type channel_type);
int wlan_hdd_cfg80211_start_bss(struct hdd_adapter *adapter,
		struct cfg80211_beacon_data *params,
		const u8 *ssid, size_t ssid_len,
		enum nl80211_hidden_ssid hidden_ssid,
		bool check_for_concurrency);

#if !defined(REMOVE_PKT_LOG)
int hdd_process_pktlog_command(struct hdd_context *hdd_ctx, uint32_t set_value,
			       int set_value2);
int hdd_pktlog_enable_disable(struct hdd_context *hdd_ctx, bool enable,
			      uint8_t user_triggered, int size);

#else
static inline
int hdd_pktlog_enable_disable(struct hdd_context *hdd_ctx, bool enable,
			      uint8_t user_triggered, int size)
{
	return 0;
}

static inline
int hdd_process_pktlog_command(struct hdd_context *hdd_ctx,
			       uint32_t set_value, int set_value2)
{
	return 0;
}
#endif /* REMOVE_PKT_LOG */

#if defined(FEATURE_SG) && !defined(CONFIG_HL_SUPPORT)
/**
 * hdd_set_sg_flags() - enable SG flag in the network device
 * @hdd_ctx: HDD context
 * @wlan_dev: network device structure
 *
 * This function enables the SG feature flag in the
 * given network device.
 *
 * Return: none
 */
static inline void hdd_set_sg_flags(struct hdd_context *hdd_ctx,
				struct net_device *wlan_dev)
{
	hdd_debug("SG Enabled");
	wlan_dev->features |= NETIF_F_SG;
}
#else
static inline void hdd_set_sg_flags(struct hdd_context *hdd_ctx,
				struct net_device *wlan_dev){}
#endif

#ifdef FEATURE_TSO
/**
 * hdd_set_tso_flags() - enable TSO flags in the network device
 * @hdd_ctx: HDD context
 * @wlan_dev: network device structure
 *
 * This function enables the TSO related feature flags in the
 * given network device.
 *
 * Return: none
 */
static inline void hdd_set_tso_flags(struct hdd_context *hdd_ctx,
	 struct net_device *wlan_dev)
{
	if (cdp_cfg_get(cds_get_context(QDF_MODULE_ID_SOC),
			cfg_dp_tso_enable) &&
			cdp_cfg_get(cds_get_context(QDF_MODULE_ID_SOC),
				    cfg_dp_enable_ip_tcp_udp_checksum_offload)){
	    /*
	     * We want to enable TSO only if IP/UDP/TCP TX checksum flag is
	     * enabled.
	     */
		hdd_debug("TSO Enabled");
		wlan_dev->features |=
			 NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM |
			 NETIF_F_TSO | NETIF_F_TSO6 | NETIF_F_SG;
	}
}
#else
static inline void hdd_set_tso_flags(struct hdd_context *hdd_ctx,
	 struct net_device *wlan_dev)
{
	hdd_set_sg_flags(hdd_ctx, wlan_dev);
}
#endif /* FEATURE_TSO */

void hdd_get_ibss_peer_info_cb(void *context,
				tSirPeerInfoRspParams *peer_info);

#ifdef CONFIG_CNSS_LOGGER
/**
 * wlan_hdd_nl_init() - wrapper function to CNSS_LOGGER case
 * @hdd_ctx:	the hdd context pointer
 *
 * The nl_srv_init() will call to cnss_logger_device_register() and
 * expect to get a radio_index from cnss_logger module and assign to
 * hdd_ctx->radio_index, then to maintain the consistency to original
 * design, adding the radio_index check here, then return the error
 * code if radio_index is not assigned correctly, which means the nl_init
 * from cnss_logger is failed.
 *
 * Return: 0 if successfully, otherwise error code
 */
static inline int wlan_hdd_nl_init(struct hdd_context *hdd_ctx)
{
	hdd_ctx->radio_index = nl_srv_init(hdd_ctx->wiphy);

	/* radio_index is assigned from 0, so only >=0 will be valid index  */
	if (hdd_ctx->radio_index >= 0)
		return 0;
	else
		return -EINVAL;
}
#else
/**
 * wlan_hdd_nl_init() - wrapper function to non CNSS_LOGGER case
 * @hdd_ctx:	the hdd context pointer
 *
 * In case of non CNSS_LOGGER case, the nl_srv_init() will initialize
 * the netlink socket and return the success or not.
 *
 * Return: the return value from  nl_srv_init()
 */
static inline int wlan_hdd_nl_init(struct hdd_context *hdd_ctx)
{
	return nl_srv_init(hdd_ctx->wiphy);
}
#endif
QDF_STATUS hdd_sme_open_session_callback(uint8_t vdev_id,
					 QDF_STATUS qdf_status);
QDF_STATUS hdd_sme_close_session_callback(uint8_t vdev_id);

int hdd_reassoc(struct hdd_adapter *adapter, const uint8_t *bssid,
		uint8_t channel, const handoff_src src);
int hdd_register_cb(struct hdd_context *hdd_ctx);
void hdd_deregister_cb(struct hdd_context *hdd_ctx);
int hdd_start_station_adapter(struct hdd_adapter *adapter);
int hdd_start_ap_adapter(struct hdd_adapter *adapter);
int hdd_configure_cds(struct hdd_context *hdd_ctx);
int hdd_set_fw_params(struct hdd_adapter *adapter);

/**
 * hdd_wlan_start_modules() - Single driver state machine for starting modules
 * @hdd_ctx: HDD context
 * @reinit: flag to indicate from SSR or normal path
 *
 * This function maintains the driver state machine it will be invoked from
 * startup, reinit and change interface. Depending on the driver state shall
 * perform the opening of the modules.
 *
 * Return: Errno
 */
int hdd_wlan_start_modules(struct hdd_context *hdd_ctx, bool reinit);

/**
 * hdd_wlan_stop_modules - Single driver state machine for stoping modules
 * @hdd_ctx: HDD context
 * @ftm_mode: ftm mode
 *
 * This function maintains the driver state machine it will be invoked from
 * exit, shutdown and con_mode change handler. Depending on the driver state
 * shall perform the stopping/closing of the modules.
 *
 * Return: Errno
 */
int hdd_wlan_stop_modules(struct hdd_context *hdd_ctx, bool ftm_mode);

/**
 * hdd_psoc_idle_timer_start() - start the idle psoc detection timer
 * @hdd_ctx: the hdd context for which the timer should be started
 *
 * Return: None
 */
void hdd_psoc_idle_timer_start(struct hdd_context *hdd_ctx);

/**
 * hdd_psoc_idle_timer_stop() - stop the idle psoc detection timer
 * @hdd_ctx: the hdd context for which the timer should be stopped
 *
 * Return: None
 */
void hdd_psoc_idle_timer_stop(struct hdd_context *hdd_ctx);

/**
 * hdd_trigger_psoc_idle_restart() - trigger restart of a previously shutdown
 *                                   idle psoc, if needed
 * @hdd_ctx: the hdd context which should be restarted
 *
 * This API does nothing if the given psoc is already active.
 *
 * Return: Errno
 */
int hdd_trigger_psoc_idle_restart(struct hdd_context *hdd_ctx);

int hdd_start_adapter(struct hdd_adapter *adapter);
void hdd_populate_random_mac_addr(struct hdd_context *hdd_ctx, uint32_t num);
/**
 * hdd_is_interface_up()- Checkfor interface up before ssr
 * @hdd_ctx: HDD context
 *
 * check  if there are any wlan interfaces before SSR accordingly start
 * the interface.
 *
 * Return: 0 if interface was opened else false
 */
bool hdd_is_interface_up(struct hdd_adapter *adapter);

void hdd_connect_result(struct net_device *dev, const u8 *bssid,
			struct csr_roam_info *roam_info, const u8 *req_ie,
			size_t req_ie_len, const u8 *resp_ie,
			size_t resp_ie_len, u16 status, gfp_t gfp,
			bool connect_timeout,
			tSirResultCodes timeout_reason);

#ifdef WLAN_FEATURE_FASTPATH
void hdd_enable_fastpath(struct hdd_context *hdd_ctx,
			 void *context);
#else
static inline void hdd_enable_fastpath(struct hdd_context *hdd_ctx,
				       void *context)
{
}
#endif
void hdd_wlan_update_target_info(struct hdd_context *hdd_ctx, void *context);

enum  sap_acs_dfs_mode wlan_hdd_get_dfs_mode(enum dfs_mode mode);
void hdd_unsafe_channel_restart_sap(struct hdd_context *hdd_ctx);
/**
 * hdd_clone_local_unsafe_chan() - clone hdd ctx unsafe chan list
 * @hdd_ctx: hdd context pointer
 * @local_unsafe_list: copied unsafe chan list array
 * @local_unsafe_list_count: channel number in returned local_unsafe_list
 *
 * The function will allocate memory and make a copy the current unsafe
 * channels from hdd ctx. The caller need to free the local_unsafe_list
 * memory after use.
 *
 * Return: 0 if successfully clone unsafe chan list.
 */
int hdd_clone_local_unsafe_chan(struct hdd_context *hdd_ctx,
	uint16_t **local_unsafe_list, uint16_t *local_unsafe_list_count);

/**
 * hdd_local_unsafe_channel_updated() - check unsafe chan list same or not
 * @hdd_ctx: hdd context pointer
 * @local_unsafe_list: unsafe chan list to be compared with hdd_ctx's list
 * @local_unsafe_list_count: channel number in local_unsafe_list
 *
 * The function checked the input channel is same as current unsafe chan
 * list in hdd_ctx.
 *
 * Return: true if input channel list is same as the list in hdd_ctx
 */
bool hdd_local_unsafe_channel_updated(struct hdd_context *hdd_ctx,
	uint16_t *local_unsafe_list, uint16_t local_unsafe_list_count);

int hdd_enable_disable_ca_event(struct hdd_context *hddctx,
				uint8_t set_value);
void wlan_hdd_undo_acs(struct hdd_adapter *adapter);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 7, 0))
static inline int
hdd_wlan_nla_put_u64(struct sk_buff *skb, int attrtype, u64 value)
{
	return nla_put_u64(skb, attrtype, value);
}
#else
static inline int
hdd_wlan_nla_put_u64(struct sk_buff *skb, int attrtype, u64 value)
{
	return nla_put_u64_64bit(skb, attrtype, value, NL80211_ATTR_PAD);
}
#endif

/**
 * hdd_roam_profile() - Get adapter's roam profile
 * @adapter: The adapter being queried
 *
 * Given an adapter this function returns a pointer to its roam profile.
 *
 * NOTE WELL: Caller is responsible for ensuring this interface is only
 * invoked for STA-type interfaces
 *
 * Return: pointer to the adapter's roam profile
 */
static inline
struct csr_roam_profile *hdd_roam_profile(struct hdd_adapter *adapter)
{
	struct hdd_station_ctx *sta_ctx;

	sta_ctx = WLAN_HDD_GET_STATION_CTX_PTR(adapter);

	return &sta_ctx->roam_profile;
}

/**
 * hdd_security_ie() - Get adapter's security IE
 * @adapter: The adapter being queried
 *
 * Given an adapter this function returns a pointer to its security IE
 * buffer. Note that this buffer is maintained outside the roam
 * profile but, when in use, is referenced by a pointer within the
 * roam profile.
 *
 * NOTE WELL: Caller is responsible for ensuring this interface is only
 * invoked for STA-type interfaces
 *
 * Return: pointer to the adapter's roam profile security IE buffer
 */
static inline
uint8_t *hdd_security_ie(struct hdd_adapter *adapter)
{
	struct hdd_station_ctx *sta_ctx;

	sta_ctx = WLAN_HDD_GET_STATION_CTX_PTR(adapter);

	return sta_ctx->security_ie;
}

/**
 * hdd_assoc_additional_ie() - Get adapter's assoc additional IE
 * @adapter: The adapter being queried
 *
 * Given an adapter this function returns a pointer to its assoc
 * additional IE buffer. Note that this buffer is maintained outside
 * the roam profile but, when in use, is referenced by a pointer
 * within the roam profile.
 *
 * NOTE WELL: Caller is responsible for ensuring this interface is only
 * invoked for STA-type interfaces
 *
 * Return: pointer to the adapter's assoc additional IE buffer
 */
static inline
tSirAddie *hdd_assoc_additional_ie(struct hdd_adapter *adapter)
{
	struct hdd_station_ctx *sta_ctx;

	sta_ctx = WLAN_HDD_GET_STATION_CTX_PTR(adapter);

	return &sta_ctx->assoc_additional_ie;
}

/**
 * hdd_is_roaming_in_progress() - check if roaming is in progress
 * @hdd_ctx - Global HDD context
 *
 * Checks if roaming is in progress on any of the adapters
 *
 * Return: true if roaming is in progress else false
 */
bool hdd_is_roaming_in_progress(struct hdd_context *hdd_ctx);
void hdd_set_roaming_in_progress(bool value);

/**
 * hdd_is_connection_in_progress() - check if connection is in progress
 * @out_vdev_id: id of vdev where connection is occurring
 * @out_reason: scan reject reason
 *
 * Go through each adapter and check if connection is in progress.
 * Output parameters @out_vdev_id and @out_reason will only be written
 * when a connection is in progress.
 *
 * Return: true if connection is in progress else false
 */
bool hdd_is_connection_in_progress(uint8_t *out_vdev_id,
				   enum scan_reject_states *out_reason);

void hdd_restart_sap(struct hdd_adapter *ap_adapter);
void hdd_check_and_restart_sap_with_non_dfs_acs(void);
bool hdd_set_connection_in_progress(bool value);

/**
 * wlan_hdd_init_chan_info() - initialize channel info variables
 * @hdd_ctx: hdd ctx
 *
 * This API initialize channel info variables
 *
 * Return: None
 */
void wlan_hdd_init_chan_info(struct hdd_context *hdd_ctx);

/**
 * wlan_hdd_deinit_chan_info() - deinitialize channel info variables
 * @hdd_ctx: hdd ctx
 *
 * This API deinitialize channel info variables
 *
 * Return: None
 */
void wlan_hdd_deinit_chan_info(struct hdd_context *hdd_ctx);
void wlan_hdd_start_sap(struct hdd_adapter *ap_adapter, bool reinit);

/**
 * hdd_is_any_interface_open() - Check for interface up
 * @hdd_ctx: HDD context
 *
 * Return: true if any interface is open
 */
bool hdd_is_any_interface_open(struct hdd_context *hdd_ctx);

#ifdef WIFI_POS_CONVERGED
/**
 * hdd_send_peer_status_ind_to_app() - wrapper to call legacy or new wifi_pos
 * function to send peer status to a registered application
 * @peer_mac: MAC address of peer
 * @peer_status: ePeerConnected or ePeerDisconnected
 * @peer_timing_meas_cap: 0: RTT/RTT2, 1: RTT3. Default is 0
 * @vdev_id: ID of the underlying vdev
 * @chan_info: operating channel information
 * @dev_mode: dev mode for which indication is sent
 *
 * Return: none
 */
static inline void hdd_send_peer_status_ind_to_app(
					struct qdf_mac_addr *peer_mac,
					uint8_t peer_status,
					uint8_t peer_timing_meas_cap,
					uint8_t vdev_id,
					struct oem_channel_info *chan_info,
					enum QDF_OPMODE dev_mode)
{
	struct wifi_pos_ch_info ch_info;

	if (!chan_info) {
		os_if_wifi_pos_send_peer_status(peer_mac, peer_status,
						peer_timing_meas_cap, vdev_id,
						NULL, dev_mode);
		return;
	}

	ch_info.chan_id = chan_info->chan_id;
	ch_info.mhz = chan_info->mhz;
	ch_info.band_center_freq1 = chan_info->band_center_freq1;
	ch_info.band_center_freq2 = chan_info->band_center_freq2;
	ch_info.info = chan_info->info;
	ch_info.reg_info_1 = chan_info->reg_info_1;
	ch_info.reg_info_2 = chan_info->reg_info_2;
	ch_info.nss = chan_info->nss;
	ch_info.rate_flags = chan_info->rate_flags;
	ch_info.sec_ch_offset = chan_info->sec_ch_offset;
	ch_info.ch_width = chan_info->ch_width;
	os_if_wifi_pos_send_peer_status(peer_mac, peer_status,
					peer_timing_meas_cap, vdev_id,
					&ch_info, dev_mode);
}
#else
static inline void hdd_send_peer_status_ind_to_app(
					struct qdf_mac_addr *peer_mac,
					uint8_t peer_status,
					uint8_t peer_timing_meas_cap,
					uint8_t vdev_id,
					struct oem_channel_info *chan_info,
					enum QDF_OPMODE dev_mode)
{
	hdd_send_peer_status_ind_to_oem_app(peer_mac, peer_status,
			peer_timing_meas_cap, vdev_id, chan_info, dev_mode);
}
#endif /* WIFI_POS_CONVERGENCE */

/**
 * wlan_hdd_send_p2p_quota()- Send P2P Quota value to FW
 * @adapter: Adapter data
 * @sval:    P2P quota value
 *
 * Send P2P quota value to FW
 *
 * Return: 0 success else failure
 */
int wlan_hdd_send_p2p_quota(struct hdd_adapter *adapter, int sval);

/**
 * wlan_hdd_send_p2p_quota()- Send MCC latency to FW
 * @adapter: Adapter data
 * @sval:    MCC latency value
 *
 * Send MCC latency value to FW
 *
 * Return: 0 success else failure
 */
int wlan_hdd_send_mcc_latency(struct hdd_adapter *adapter, int sval);

/**
 * wlan_hdd_get_adapter_from_vdev()- Get adapter from vdev id
 * and PSOC object data
 * @psoc: Psoc object data
 * @vdev_id: vdev id
 *
 * Get adapter from vdev id and PSOC object data
 *
 * Return: adapter pointer
 */
struct hdd_adapter *wlan_hdd_get_adapter_from_vdev(struct wlan_objmgr_psoc
					*psoc, uint8_t vdev_id);
/**
 * hdd_unregister_notifiers()- unregister kernel notifiers
 * @hdd_ctx: Hdd Context
 *
 * Unregister netdev notifiers like Netdevice,IPv4 and IPv6.
 *
 */
void hdd_unregister_notifiers(struct hdd_context *hdd_ctx);

/**
 * hdd_dbs_scan_selection_init() - initialization for DBS scan selection config
 * @hdd_ctx: HDD context
 *
 * This function sends the DBS scan selection config configuration to the
 * firmware via WMA
 *
 * Return: 0 - success, < 0 - failure
 */
int hdd_dbs_scan_selection_init(struct hdd_context *hdd_ctx);

/**
 * hdd_start_complete()- complete the start event
 * @ret: return value for complete event.
 *
 * complete the startup event and set the return in
 * global variable
 *
 * Return: void
 */

void hdd_start_complete(int ret);

/**
 * hdd_chip_pwr_save_fail_detected_cb() - chip power save failure detected
 * callback
 * @hdd_handle: HDD handle
 * @data: chip power save failure detected data
 *
 * This function reads the chip power save failure detected data and fill in
 * the skb with NL attributes and send up the NL event.
 * This callback execute in atomic context and must not invoke any
 * blocking calls.
 *
 * Return: none
 */

void hdd_chip_pwr_save_fail_detected_cb(hdd_handle_t hdd_handle,
				struct chip_pwr_save_fail_detected_params
				*data);

/**
 * hdd_update_ie_whitelist_attr() - Copy probe req ie whitelist attrs from cfg
 * @ie_whitelist: output parameter
 * @hdd_ctx: pointer to hdd context
 *
 * Return: None
 */
void hdd_update_ie_whitelist_attr(struct probe_req_whitelist_attr *ie_whitelist,
				  struct hdd_context *hdd_ctx);

/**
 * hdd_get_rssi_snr_by_bssid() - gets the rssi and snr by bssid from scan cache
 * @adapter: adapter handle
 * @bssid: bssid to look for in scan cache
 * @rssi: rssi value found
 * @snr: snr value found
 *
 * Return: QDF_STATUS
 */
int hdd_get_rssi_snr_by_bssid(struct hdd_adapter *adapter, const uint8_t *bssid,
			      int8_t *rssi, int8_t *snr);

/**
 * hdd_reset_limit_off_chan() - reset limit off-channel command parameters
 * @adapter - HDD adapter
 *
 * Return: 0 on success and non zero value on failure
 */
int hdd_reset_limit_off_chan(struct hdd_adapter *adapter);

#if defined(WLAN_FEATURE_FILS_SK) && \
	(defined(CFG80211_FILS_SK_OFFLOAD_SUPPORT) || \
		 (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)))
/**
 * hdd_clear_fils_connection_info: API to clear fils info from roam profile and
 * free allocated memory
 * @adapter: pointer to hdd adapter
 *
 * Return: None
 */
void hdd_clear_fils_connection_info(struct hdd_adapter *adapter);

/**
 * hdd_update_hlp_info() - Update HLP packet received in FILS (re)assoc rsp
 * @dev: net device
 * @roam_fils_params: Fils join rsp params
 *
 * This API is used to send the received HLP packet in Assoc rsp(FILS AKM)
 * to the network layer.
 *
 * Return: None
 */
void hdd_update_hlp_info(struct net_device *dev,
			 struct csr_roam_info *roam_info);
#else
static inline void hdd_clear_fils_connection_info(struct hdd_adapter *adapter)
{ }
static inline void hdd_update_hlp_info(struct net_device *dev,
				       struct csr_roam_info *roam_info)
{}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 12, 0)
static inline void hdd_dev_setup_destructor(struct net_device *dev)
{
	dev->destructor = free_netdev;
}
#else
static inline void hdd_dev_setup_destructor(struct net_device *dev)
{
	dev->needs_free_netdev = true;
}
#endif /* KERNEL_VERSION(4, 12, 0) */

/**
 * hdd_dp_trace_init() - initialize DP Trace by calling the QDF API
 * @config: hdd config
 *
 * Return: NONE
 */
#ifdef CONFIG_DP_TRACE
void hdd_dp_trace_init(struct hdd_config *config);
#else
static inline
void hdd_dp_trace_init(struct hdd_config *config) {}
#endif

/**
 * hdd_set_rx_mode_rps() - Enable/disable RPS in SAP mode
 * @enable: Set true to enable RPS in SAP mode
 *
 * Callback function registered with datapath
 *
 * Return: none
 */
void hdd_set_rx_mode_rps(bool enable);

/**
 * hdd_limit_max_per_index_score() -check if per index score doesn't exceed 100%
 * (0x64). If it exceed make it 100%
 *
 * @per_index_score: per_index_score as input
 *
 * Return: per_index_score within the max limit
 */
uint32_t hdd_limit_max_per_index_score(uint32_t per_index_score);

/**
 * hdd_update_score_config - API to update candidate scoring related params
 * configuration parameters
 * @score_config: score config to update
 * @cfg: config params
 *
 * Return: QDF_STATUS
 */
QDF_STATUS hdd_update_score_config(
	struct scoring_config *score_config, struct hdd_context *hdd_ctx);

/**
 * hdd_get_stainfo() - get stainfo for the specified peer
 * @astainfo: array of the station info in which the sta info
 * corresponding to mac_addr needs to be searched
 * @mac_addr: mac address of requested peer
 *
 * This function find the stainfo for the peer with mac_addr
 *
 * Return: stainfo if found, NULL if not found
 */
struct hdd_station_info *hdd_get_stainfo(struct hdd_station_info *astainfo,
					 struct qdf_mac_addr mac_addr);

/**
 * hdd_component_psoc_open() - Open the legacy components
 * @psoc: Pointer to psoc object
 *
 * This function opens the legacy components and initializes the
 * component's private objects.
 *
 * Return: QDF_STATUS
 */
QDF_STATUS hdd_component_psoc_open(struct wlan_objmgr_psoc *psoc);

/**
 * hdd_component_psoc_close() - Close the legacy components
 * @psoc: Pointer to psoc object
 *
 * This function closes the legacy components and resets the
 * component's private objects.
 *
 * Return: None
 */
void hdd_component_psoc_close(struct wlan_objmgr_psoc *psoc);

/**
 * hdd_component_psoc_enable() - Trigger psoc enable for CLD Components
 *
 * Return: None
 */
void hdd_component_psoc_enable(struct wlan_objmgr_psoc *psoc);

/**
 * hdd_component_psoc_disable() - Trigger psoc disable for CLD Components
 *
 * Return: None
 */
void hdd_component_psoc_disable(struct wlan_objmgr_psoc *psoc);

/**
 * hdd_component_pdev_open() - Trigger pdev open for CLD Components
 *
 * Return: QDF_STATUS
 */
QDF_STATUS hdd_component_pdev_open(struct wlan_objmgr_pdev *pdev);

/**
 * hdd_component_pdev_close() - Trigger pdev close for CLD Components
 *
 * Return: None
 */
void hdd_component_pdev_close(struct wlan_objmgr_pdev *pdev);

#ifdef WLAN_FEATURE_MEMDUMP_ENABLE
int hdd_driver_memdump_init(void);
void hdd_driver_memdump_deinit(void);
#else /* WLAN_FEATURE_MEMDUMP_ENABLE */
static inline int hdd_driver_memdump_init(void)
{
	return 0;
}
static inline void hdd_driver_memdump_deinit(void)
{
}
#endif /* WLAN_FEATURE_MEMDUMP_ENABLE */
/**
 * hdd_set_disconnect_status() - set adapter disconnection status
 * @hdd_adapter: Pointer to hdd adapter
 * @disconnecting: Disconnect status to set
 *
 * Return: None
 */
void hdd_set_disconnect_status(struct hdd_adapter *adapter, bool disconnecting);

#ifdef FEATURE_MONITOR_MODE_SUPPORT
/**
 * wlan_hdd_set_mon_chan() - Set capture channel on the monitor mode interface.
 * @adapter: Handle to adapter
 * @chan: Monitor mode channel
 * @bandwidth: Capture channel bandwidth
 *
 * Return: 0 on success else error code.
 */
int wlan_hdd_set_mon_chan(struct hdd_adapter *adapter, uint32_t chan,
			  uint32_t bandwidth);
#else
static inline
int wlan_hdd_set_mon_chan(struct hdd_adapter *adapter, uint32_t chan,
			  uint32_t bandwidth)
{
	return 0;
}
#endif

/**
 * hdd_wlan_get_version() - Get version information
 * @hdd_ctx: Global HDD context
 * @version_len: length of the version buffer size
 * @version: the buffer to the version string
 *
 * This function is used to get Wlan Driver, Firmware, Hardware Version
 * & the Board related information.
 *
 * Return: the length of the version string
 */
uint32_t hdd_wlan_get_version(struct hdd_context *hdd_ctx,
			      const size_t version_len, uint8_t *version);
/**
 * hdd_assemble_rate_code() - assemble rate code to be sent to FW
 * @preamble: rate preamble
 * @nss: number of streams
 * @rate: rate index
 *
 * Rate code assembling is different for targets which are 11ax capable.
 * Check for the target support and assemble the rate code accordingly.
 *
 * Return: assembled rate code
 */
int hdd_assemble_rate_code(uint8_t preamble, uint8_t nss, uint8_t rate);

/**
 * hdd_set_11ax_rate() - set 11ax rate
 * @adapter: adapter being modified
 * @value: new 11ax rate code
 * @sap_config: pointer to SAP config to check HW mode
 *		this will be NULL for call from STA persona
 *
 * Return: 0 on success, negative errno on failure
 */
int hdd_set_11ax_rate(struct hdd_adapter *adapter, int value,
		      struct sap_config *sap_config);

/**
 * hdd_update_hw_sw_info() - API to update the HW/SW information
 * @hdd_ctx: Global HDD context
 *
 * API to update the HW and SW information in the driver
 *
 * Note:
 * All the version/revision information would only be retrieved after
 * firmware download
 *
 * Return: None
 */
void hdd_update_hw_sw_info(struct hdd_context *hdd_ctx);

/**
 * hdd_get_nud_stats_cb() - callback api to update the stats received from FW
 * @data: pointer to hdd context.
 * @rsp: pointer to data received from FW.
 * @context: callback context
 *
 * This is called when wlan driver received response event for
 * get arp stats to firmware.
 *
 * Return: None
 */
void hdd_get_nud_stats_cb(void *data, struct rsp_stats *rsp, void *context);

/**
 * hdd_context_get_mac_handle() - get mac handle from hdd context
 * @hdd_ctx: Global HDD context pointer
 *
 * Retrieves the global MAC handle from the HDD context
 *
 * Return: The global MAC handle (which may be NULL)
 */
static inline
mac_handle_t hdd_context_get_mac_handle(struct hdd_context *hdd_ctx)
{
	return hdd_ctx ? hdd_ctx->mac_handle : NULL;
}

/**
 * hdd_adapter_get_mac_handle() - get mac handle from hdd adapter
 * @adapter: HDD adapter pointer
 *
 * Retrieves the global MAC handle given an HDD adapter
 *
 * Return: The global MAC handle (which may be NULL)
 */
static inline
mac_handle_t hdd_adapter_get_mac_handle(struct hdd_adapter *adapter)
{
	return adapter ?
		hdd_context_get_mac_handle(adapter->hdd_ctx) : NULL;
}

/**
 * hdd_handle_to_context() - turn an HDD handle into an HDD context
 * @hdd_handle: HDD handle to be converted
 *
 * Return: HDD context referenced by @hdd_handle
 */
static inline
struct hdd_context *hdd_handle_to_context(hdd_handle_t hdd_handle)
{
	return (struct hdd_context *)hdd_handle;
}

/**
 * wlan_hdd_free_cache_channels() - Free the cache channels list
 * @hdd_ctx: Pointer to HDD context
 *
 * Return: None
 */
void wlan_hdd_free_cache_channels(struct hdd_context *hdd_ctx);

/**
 * hdd_update_dynamic_mac() - Updates the dynamic MAC list
 * @hdd_ctx: Pointer to HDD context
 * @curr_mac_addr: Current interface mac address
 * @new_mac_addr: New mac address which needs to be updated
 *
 * This function updates newly configured MAC address to the
 * dynamic MAC address list corresponding to the current
 * adapter MAC address
 *
 * Return: None
 */
void hdd_update_dynamic_mac(struct hdd_context *hdd_ctx,
			    struct qdf_mac_addr *curr_mac_addr,
			    struct qdf_mac_addr *new_mac_addr);

#ifdef WLAN_FEATURE_DP_BUS_BANDWIDTH
/**
 * wlan_hdd_send_tcp_param_update_event() - Send vendor event to update
 * TCP parameter through Wi-Fi HAL
 * @hdd_ctx: Pointer to HDD context
 * @data: Parameters to update
 * @dir: Direction(tx/rx) to update
 *
 * Return: None
 */
void wlan_hdd_send_tcp_param_update_event(struct hdd_context *hdd_ctx,
					  void *data,
					  uint8_t dir);

/**
 * wlan_hdd_update_tcp_rx_param() - update TCP param in RX dir
 * @hdd_ctx: Pointer to HDD context
 * @data: Parameters to update
 *
 * Return: None
 */
void wlan_hdd_update_tcp_rx_param(struct hdd_context *hdd_ctx, void *data);

/**
 * wlan_hdd_update_tcp_tx_param() - update TCP param in TX dir
 * @hdd_ctx: Pointer to HDD context
 * @data: Parameters to update
 *
 * Return: None
 */
void wlan_hdd_update_tcp_tx_param(struct hdd_context *hdd_ctx, void *data);
#else
static inline
void wlan_hdd_update_tcp_rx_param(struct hdd_context *hdd_ctx, void *data)
{
}

static inline
void wlan_hdd_update_tcp_tx_param(struct hdd_context *hdd_ctx, void *data)
{
}

static inline
void wlan_hdd_send_tcp_param_update_event(struct hdd_context *hdd_ctx,
					  void *data,
					  uint8_t dir)
{
}
#endif /*WLAN_FEATURE_DP_BUS_BANDWIDTH*/

#ifdef WLAN_FEATURE_MOTION_DETECTION
/**
 * hdd_md_host_evt_cb - Callback for Motion Detection Event
 * @ctx: HDD context
 * @sir_md_evt: motion detect event
 *
 * Callback for Motion Detection Event. Re-enables Motion
 * Detection again upon event
 *
 * Return: QDF_STATUS QDF_STATUS_SUCCESS on Success and
 * QDF_STATUS_E_FAILURE on failure
 */
QDF_STATUS hdd_md_host_evt_cb(void *ctx, struct sir_md_evt *event);
#endif /* WLAN_FEATURE_MOTION_DETECTION */

/**
 * hdd_hidden_ssid_enable_roaming() - enable roaming after hidden ssid rsp
 * @hdd_handle: Hdd handler
 * @vdev_id: Vdev Id
 *
 * This is a wrapper function to enable roaming after getting hidden
 * ssid rsp
 */
void hdd_hidden_ssid_enable_roaming(hdd_handle_t hdd_handle, uint8_t vdev_id);

/**
 * hdd_send_update_owe_info_event - Send update OWE info event
 * @adapter: Pointer to adapter
 * @sta_addr: MAC address of peer STA
 * @owe_ie: OWE IE
 * @owe_ie_len: Length of OWE IE
 *
 * Send update OWE info event to hostapd
 *
 * Return: none
 */
#ifdef CFG80211_EXTERNAL_DH_UPDATE_SUPPORT
void hdd_send_update_owe_info_event(struct hdd_adapter *adapter,
				    uint8_t sta_addr[],
				    uint8_t *owe_ie,
				    uint32_t owe_ie_len);
#else
static inline void hdd_send_update_owe_info_event(struct hdd_adapter *adapter,
						  uint8_t sta_addr[],
						  uint8_t *owe_ie,
						  uint32_t owe_ie_len)
{
}
#endif

/**
 * hdd_psoc_idle_shutdown - perform idle shutdown after interface inactivity
 *                          timeout
 * @device: pointer to struct device
 *
 * Return: 0 for success non-zero error code for failure
 */
int hdd_psoc_idle_shutdown(struct device *dev);

/**
 * hdd_psoc_idle_restart - perform idle restart after idle shutdown
 * @device: pointer to struct device
 *
 * Return: 0 for success non-zero error code for failure
 */
int hdd_psoc_idle_restart(struct device *dev);

#endif /* end #if !defined(WLAN_HDD_MAIN_H) */