/*
MIT License

Copyright (c) 2022 Rajaram Regupathy <rajaram.regupathy@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
// SPDX-License-Identifier: MIT
/**
 * @file libtypec.h
 * @author Rajaram Regupathy <rajaram.regupathy@gmail.com>
 * @brief
 *      Public libtypec header file providing interfaces for USB Type-C
 * Connector System Software.
 *
 */

#ifndef LIBTYPEC_H
#define LIBTYPEC_H

#include <stdint.h>
#include "libtypec_config.h"

union optionalfeature {
    struct
    {
        unsigned char setccomsupported : 1;
        unsigned char setpowerlevelsupported : 1;
        unsigned char altmodedetailssupported : 1;
        unsigned char altmodeoverridesupported : 1;
        unsigned char pdodetailssupported : 1;
        unsigned char cabledetailssupported : 1;
        unsigned char extsupplynotificationsupported : 1;
        unsigned char pdresetnotificationsupported : 1;
        unsigned char getpdmessagesupported : 1;
        unsigned char getattentionvdosupported : 1;
        unsigned char fwupdaterequestsupported : 1;
        unsigned char negotiatedpowerlevelchangesupported : 1;
        unsigned char securityrequestsupported : 1;
        unsigned char setretimermodesupported : 1;
        unsigned char chunkingsupportsupported : 1;
        unsigned char reserved1 : 1;
        unsigned char reserved2;
    };
    unsigned char raw_optfeas[3];
};
union powersource {
    struct
    {
        unsigned char acsupply : 1;
        unsigned char reserved1 : 1;
        unsigned char other : 1;
        unsigned char reserved2 : 3;
        unsigned char vbus : 1;
        unsigned char reserved3 : 1;
    };
    unsigned char raw_powersrc;
};

union attributes {
    struct {
        unsigned int disabledstatesupport : 1;
        unsigned int batterycharging : 1;
        unsigned int usbpowerdelivery : 1;
        unsigned int reserved1 : 3;
        unsigned int usbtypeccurrent : 1;
        unsigned int reserved2 : 1;
        union powersource bmPowerSource;
        unsigned int reserved3 : 16;
    };
    unsigned int raw_attrs;
};

struct libtypec_capability_data {
    union attributes bmAttributes;
    unsigned int bNumConnectors : 7;
    unsigned int reserved1 : 1;
    union optionalfeature bmOptionalFeatures;
    unsigned int bNumAltModes : 8;
    unsigned int reserved2 : 8;
    unsigned int bcdBCVersion : 16;
    unsigned int bcdPDVersion : 16;
    unsigned int bcdTypeCVersion : 16;
};

union operationmode {
    struct
    {
        unsigned char rponly : 1;
        unsigned char rdonly : 1;
        unsigned char drp : 1;
        unsigned char analogaudioaccessorymode : 1;
        unsigned char debugaccessorymode : 1;
        unsigned char usb2 : 1;
        unsigned char usb3 : 1;
        unsigned char alternatemode : 1;
    };
    unsigned char raw_operationmode;
};

union extendedoperationmode {
    struct
    {
        unsigned char usb4gen2 : 1;
        unsigned char eprsrc : 1;
        unsigned char eprsnk : 1;
        unsigned char usb4gen3 : 1;
        unsigned char usb4gen4 : 1;
        unsigned char reserved : 3;
    };
    unsigned char raw_extendedoperationmode;
};

union miscellaneouscapabilities {
    struct
    {
        unsigned char fwupdate : 1;
        unsigned char security : 1;
        unsigned char reserved : 2;
    };
    unsigned char raw_miscellaneouscapabilities;
};
struct libtypec_connector_cap_data
{
    union operationmode opr_mode;
    unsigned int provider : 1;
    unsigned int consumer : 1;
    unsigned int swap2dfp : 1;
    unsigned int swap2ufp : 1;
    unsigned int swap2src : 1;
    unsigned int swap2snk : 1;
    unsigned int extended_operation_mode : 8;
    unsigned int miscellaneous_capabilities : 4;
    unsigned int reverse_current_protection_support : 1;
    unsigned int partner_pd_rev : 2;
    unsigned int reserved :3;
}__attribute__((packed));

struct libtypec_current_cam
{
	unsigned char current_altmode[4];
};

struct libtypec_get_pdos
{
    unsigned int pdo[4];
};
struct altmode_data
{
	uint16_t svid;
	uint32_t vdo;
};

union libtypec_discovered_identity
{
	char buf_disc_id[24];
	struct discovered_identity
	{
		uint32_t cert_stat;
		uint32_t id_header;
		uint32_t product;
		uint32_t product_type_vdo1;
		uint32_t product_type_vdo2;
		uint32_t product_type_vdo3;
	} disc_id;
};

union connectorstatuschange
{
	struct {
		unsigned char reserved1 : 1;
		unsigned char ExternalSupplyChange : 1;
		unsigned char PowerOperationModechange : 1;
		unsigned char Attention : 1;
		unsigned char Reserved2 : 1;
		unsigned char SupportedProviderCapabilitiesChange : 1;
		unsigned char NegotiatedPowerLevelChange : 1;
		unsigned char PDResetComplete : 1;
		unsigned char SupportedCAMChange : 1;
		unsigned char BatteryChargingStatusChange : 1;
		unsigned char Reserved3 : 1;
		unsigned char ConnectorPartnerChanged : 1;
		unsigned char PowerDirectionChanged : 1;
		unsigned char SinkPathStatusChange : 1;
		unsigned char ConnectChange : 1;
		unsigned char Error : 1;
	};
	unsigned short int raw_conn_stschang;
};

enum power_operation_mode {
	/** USB_DEFAULT_OPERATION */
	USB_DEFAULT_OPERATION = 1,
	/** BC_OPERATION */
	BC_OPERATION = 2,
	/** PD_OPERATION */
	PD_OPERATION = 3,
	/** USB_TC_CURRENT_1_5A */
	USB_TC_CURRENT_1_5A = 4,
	/** USB_TC_CURRENT_3A */
	USB_TC_CURRENT_3A = 5,
	/** USB_TC_CURRENT_5A */
	USB_TC_CURRENT_5A = 6,
};

union connectorpartnerflags {
	struct  {
		unsigned char usb : 1;
		unsigned char altmode : 1;
		unsigned char usb4_gen3 : 1;
		unsigned char usb4_gen4 : 1;
		unsigned char reserved : 4;
	};
	unsigned char raw_conn_part_flags;
};

enum conn_partner_type {
	/** DFP_ATTACHED */
	DFP_ATTACHED = 1,
	/** UFP_ATTACHED */
	UFP_ATTACHED = 2,
	/** POWERED_CABLE_NO_UFP_ATTACHED */
	POWERED_CABLE_NO_UFP_ATTACHED = 3,
	/** POWERED_CABLE_UFP_ATTACHED */
	POWERED_CABLE_UFP_ATTACHED = 4,
	/** DEBUG_ACCESSORY_ATTACHED */
	DEBUG_ACCESSORY_ATTACHED = 5,
	/** AUDIO_ADAPTER_ACCESSORY_ATTACHED */
	AUDIO_ADAPTER_ACCESSORY_ATTACHED = 6,
};


struct libtypec_connector_status
{
	union connectorstatuschange ConnectorStatusChange;
	unsigned int PowerOperationMode : 3;
	unsigned int ConnectStatus : 1;
	unsigned int PowerDirection : 1;
	unsigned int ConnectorPartnerFlags :8;
	unsigned int ConnectorPartnerType : 3;
	unsigned int RequestDataObject : 32;
	unsigned int BatteryChargingCapabilityStatus : 2;
	unsigned int ProviderCapabilitiesLimitedReason : 4;
	unsigned int bcdPDVersionOperationMode : 16;
	unsigned int Orientation : 1;
	unsigned int SinkPathStatus : 1;
	unsigned int ReverseCurrentProtectionStatus : 1;
	unsigned int PowerReadingReady : 1;
	unsigned int CurrentScale : 3;
	unsigned int PeakCurrent : 16;
	unsigned int AverageCurrent : 16;
	unsigned int VoltageScale : 4;
	unsigned int VoltageReading : 16;
	unsigned int Reserved : 7;
}__attribute__((packed));

struct libtypec_cable_property
{
	unsigned short speed_supported;
	unsigned int current_capability : 8;
	unsigned int vbus_support : 1;
	unsigned int cable_type : 1;
	unsigned int directionality : 1;
	unsigned int plug_end_type : 2 ;
	unsigned int mode_support : 1;
	unsigned int cable_pd_revision : 2;
	unsigned int latency : 4;
	unsigned int reserved : 28;
}__attribute__((packed));

struct libtypec_get_lpm_ppm_info
{
	unsigned short vid;
	unsigned short pid;
	unsigned int xid;
	unsigned int fw_version_upper;
	unsigned int fw_version_lower;
	unsigned int hw_version;
};

union libtypec_fixed_supply_src
{
	unsigned fixed_supply;
	struct fixed_supply_bits
	{
		unsigned max_cur:10;
		unsigned volt:10;
		unsigned peak_cur:2;
		unsigned rsvd:1;
		unsigned epr:1;
		unsigned unchunked:1;
		unsigned drd:1;
		unsigned usb_comm:1;
		unsigned uncons_pwr:1;
		unsigned usb_suspend:1;
		unsigned dual_pwr:1;
		unsigned type:2;
	}obj_fixed_sply;
};

union libtypec_variable_supply_src
{
	unsigned int variable_supply;
	struct variable_supply_bits
	{
		unsigned max_cur:10;
		unsigned min_volt:10;
        unsigned max_volt:10;
		unsigned type:2;
    }obj_var_sply;
};

union libtypec_battery_supply_src
{
	unsigned int battery_supply;
	struct battery_supply_bits
	{
		unsigned max_pwr:10;
		unsigned min_volt:10;
		unsigned max_volt:10;
		unsigned type:2;
	}obj_bat_sply;
};

union libtypec_pps_src
{
	unsigned int spr_pps_supply;
	struct pps_supply_bits
	{
		unsigned max_cur:7;
		unsigned rsvd1:1;
		unsigned min_volt:8;
		unsigned rsvd2:1;
		unsigned max_volt:8;
		unsigned rsvd3:2;
		unsigned pwr_ltd:1;
		unsigned pps_type:2;
		unsigned type:2;
	}obj_pps_sply;
};

union libtypec_fixed_supply_snk
{
    unsigned int fixed_supply;
    struct fixed_sply_bits
    {
        unsigned opr_cur:10;
        unsigned volt:10;
        unsigned rsvd:3;
        unsigned fr_swp:2;
        unsigned drd:1;
        unsigned usb_comm_cap:1;
        unsigned uncons_pwr:1;
        unsigned higher_caps:1;
        unsigned drp:1;
        unsigned type:2;
    }obj_fixed_supply;
    
};

union libtypec_variable_sply_sink
{
    unsigned int var_sply_snk;
    struct var_supply_bits
    {
        unsigned opr_cur:10;
        unsigned min_volt:10;
        unsigned max_volt:10;
        unsigned type:2;
    }obj_var_sply;
    
};

union libtypec_battery_sply_sink
{
    unsigned int battery_supply;
    struct battery_sply_bits
    {
        unsigned opr_pwr:10;
        unsigned min_volt:10;
        unsigned max_volt:10;
        unsigned type:2;
    }obj_bat_sply;
    
};

union libtypec_pps_sink
{
    unsigned int spr_pps;
    struct spr_pps_bits
    {
        unsigned max_cur:7;
        unsigned rsvd1:1;
        unsigned min_volt:8;
        unsigned rsvd2:1;
        unsigned max_volt:8;
        unsigned rsvd3:3;
        unsigned pps_type:2;
        unsigned type:2;
    }obj_spr_pps;
    
};

#define LIBTYPEC_VERSION_INDEX 0
#define LIBTYPEC_KERNEL_INDEX 1
#define LIBTYPEC_OS_INDEX 2
#define LIBTYPEC_INTF_INDEX 3
#define LIBTYPEC_OPS_INDEX 4
#define LIBTYPEC_SESSION_MAX_INDEX 5

#define OPR_MODE_RP_ONLY 0
#define OPR_MODE_RD_ONLY 1
#define OPR_MODE_DRP_ONLY 2

#define AM_CONNECTOR 0
#define AM_SOP 1
#define AM_SOP_PR 2
#define AM_SOP_DPR 3

#define PLUG_TYPE_A 0
#define PLUG_TYPE_B 1
#define PLUG_TYPE_C 2
#define PLUG_TYPE_OTH 3

#define CABLE_TYPE_PASSIVE 0
#define CABLE_TYPE_ACTIVE 1
#define CABLE_TYPE_UNKNOWN 2

#define GET_SINK_CAP_EXTENDED 0
#define GET_SOURCE_CAP_EXTENDED 1
#define GET_BATTERY_CAP 2
#define GET_BATTERY_STATUS 3
#define DISCOVER_ID_REQ 4

#define POWER_OP_MODE_PD 3
#define POWER_OP_MODE_TC_1_5 4
#define POWER_OP_MODE_TC_3 5

#define PDO_FIXED 0
#define PDO_BATTERY 1
#define PDO_VARIABLE 2
#define PDO_AUGMENTED 3

enum usb_typec_event {
    USBC_DEVICE_CONNECTED,
    USBC_DEVICE_DISCONNECTED,
    USBC_EVENT_COUNT
};
enum libtypec_backend {
    LIBTYPEC_BACKEND_SYSFS=0,
    LIBTYPEC_BACKEND_DBGFS,
    /*LIBTYPEC_BACKEND_I2C,*/ /*Potential backend interface*/
};

typedef void (*usb_typec_callback_t)(enum usb_typec_event event, void* data);

typedef struct libtypec_notification_list{
    usb_typec_callback_t cb_func;
    void* data;
    struct libtypec_notification_list* next;
} libtypec_notification_list_t;


int libtypec_init(char **session_info,enum libtypec_backend backend);
int libtypec_exit(void);

/**
 * @brief
 *
 */
int libtypec_connector_reset(int conn_num, int rst_type);
int libtypec_get_capability(struct libtypec_capability_data *cap_data);
int libtypec_get_conn_capability(int conn_num, struct libtypec_connector_cap_data *conn_cap_data);
int libtypec_get_alternate_modes(int recipient, int conn_num, struct altmode_data *alt_mode_data);
int libtypec_get_cam_supported(int conn_num, char *cam_data);
int libtypec_get_current_cam(int conn_num, struct libtypec_current_cam *cur_cam);
int libtypec_get_pdos(int conn_num, int partner, int offset, int *num_pdo, int src_snk, int type, struct libtypec_get_pdos *pdo_data);
int libtypec_get_cable_properties(int conn_num, struct libtypec_cable_property *cbl_prop_data);
int libtypec_get_connector_status(int conn_num, struct libtypec_connector_status *conn_sts);
int libtypec_get_pd_message(int recipient, int conn_num, int num_bytes, int resp_type, char *pd_msg_resp);

int libtypec_get_bb_status(unsigned int *num_bb_instance);
int libtypec_get_bb_data(int num_billboards,char* bb_data);
int libtypec_set_uor_ops(unsigned char conn_num, unsigned char uor);
int libtypec_set_pdr_ops(unsigned char conn_num, unsigned char pdr);

int libtypec_register_typec_notification_callback(enum usb_typec_event event, usb_typec_callback_t cb, void* data);
int libtypec_unregister_typec_notification_callback(enum usb_typec_event event, usb_typec_callback_t cb);
void libtypec_monitor_events(void);

#endif /*LIBTYPEC_H*/
