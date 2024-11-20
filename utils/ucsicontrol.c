/*
		Copyright (c) 2021-2022 by Madhu M, madhu.m@intel.com

		This program is free software; you can redistribute it and/or modify
		it under the terms of the GNU General Public License as published by
		the Free Software Foundation; version 2 of the License.

		This program is distributed in the hope that it will be useful, but WITHOUT
		ANY WARRANTY; without even the implied warranty MERCHANTABILITY or FITNESS
		FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
		details.

		(See the full license text in the LICENSES directory)
*/
// SPDX-License-Identifier: GPL-2.0-only
/**
 * @file ucsicontrol.c
 * @author Madhu M <madhu.m@intel.com>
 * @coauthor Rajaram Regupathy <rajaram.regupathy@gmail.com>
 * @brief Implements listing of ucsi connector details
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <getopt.h>

#include "../libtypec.h"
#include "lstypec.h"
#include "names.h"

struct libtypec_capability_data cap_data;
struct libtypec_connector_cap_data conn_cap;
struct libtypec_current_cam cur_cam;
struct libtypec_connector_status conn_sts;
struct libtypec_cable_property cable_prop;
union libtypec_discovered_identity id;
struct libtypec_get_pdos pdo_data_val;
struct altmode_data alt_mode_data[64];
char *session_info[LIBTYPEC_SESSION_MAX_INDEX];


void hex_to_decimal(unsigned int version)
{
    uint8_t major = (version >> 8) & 0xFF;
    uint8_t minor = version & 0xFF;

    printf("%x.%02x\n", major, minor);
}
/**
 * Prints the USB-C Platform Policy Manager capability data.
 *
 * @param cap_data The structure containing the capability data.
 */
void print_get_capability(struct libtypec_capability_data *cap_data)
{
	printf("UCSI MESSAGE_IN:\n");
	printf("==================\n");
	for (int i = 0; i < sizeof(*cap_data); i++) {
		printf("%02x ", ((unsigned char*)cap_data)[i]);
		if ((i + 1) % 8 == 0) {
			printf("\n");
		}
	}

	printf("\n\nUCSI_GET_CAPABILITY_IN:\n");
	printf("-------------------------\n");
	printf("bmAttributes:\n");
	printf("  disabledStateSupport: %u\n", cap_data->bmAttributes.disabledstatesupport);
	printf("  batteryCharging: %u\n", cap_data->bmAttributes.batterycharging);
	printf("  usbPowerDelivery: %u\n", cap_data->bmAttributes.usbpowerdelivery);
	printf("  usbTypeCCurrent: %u\n", cap_data->bmAttributes.usbtypeccurrent);
	printf("  bmPowerSource.acsupply: %u\n", cap_data->bmAttributes.bmPowerSource.acsupply);
	printf("  bmPowerSource.other: %u\n", cap_data->bmAttributes.bmPowerSource.other);
	printf("  bmPowerSource.vbus: %u\n", cap_data->bmAttributes.bmPowerSource.vbus);
	printf("bNumConnectors: %x\n", cap_data->bNumConnectors);
	printf("bmOptionalFeatures:\n");
	printf("  setccomsupported: %u\n", cap_data->bmOptionalFeatures.setccomsupported);
	printf("  setpowerlevelsupported: %u\n", cap_data->bmOptionalFeatures.setpowerlevelsupported);
	printf("  altmodedetailssupported: %u\n", cap_data->bmOptionalFeatures.altmodedetailssupported);
	printf("  altmodeoverridesupported: %u\n", cap_data->bmOptionalFeatures.altmodeoverridesupported);
	printf("  pdodetailssupported: %u\n", cap_data->bmOptionalFeatures.pdodetailssupported);
	printf("  cabledetailssupported: %u\n", cap_data->bmOptionalFeatures.cabledetailssupported);
	printf("  extsupplynotificationsupported: %u\n", cap_data->bmOptionalFeatures.extsupplynotificationsupported);
	printf("  pdresetnotificationsupported: %u\n", cap_data->bmOptionalFeatures.pdresetnotificationsupported);
	printf("  getpdmessagesupported: %u\n", cap_data->bmOptionalFeatures.getpdmessagesupported);
	printf("  getattentionvdosupported: %u\n", cap_data->bmOptionalFeatures.getattentionvdosupported);
	printf("  fwupdaterequestsupported: %u\n", cap_data->bmOptionalFeatures.fwupdaterequestsupported);
	printf("  negotiatedpowerlevelchangesupported: %u\n", cap_data->bmOptionalFeatures.negotiatedpowerlevelchangesupported);
	printf("  securityrequestsupported: %u\n", cap_data->bmOptionalFeatures.securityrequestsupported);
	printf("  setretimermodesupported: %u\n", cap_data->bmOptionalFeatures.setretimermodesupported);
	printf("  chunkingsupportsupported: %u\n", cap_data->bmOptionalFeatures.chunkingsupportsupported);
	printf("bNumAltModes: %x\n", cap_data->bNumAltModes);
	printf("bcdBCVersion: ");
	hex_to_decimal(cap_data->bcdBCVersion);
	printf("bcdPDVersion: ");
	hex_to_decimal(cap_data->bcdPDVersion);
	printf("bcdTypeCVersion: ");
	hex_to_decimal(cap_data->bcdTypeCVersion);
}
/**
 * Prints the USB-C Platform Policy Manager connector capability data.
 *
 * @param conn_cap The structure containing the connector capability data.
 */
void print_get_connector_capability(struct libtypec_connector_cap_data *conn_cap)
{
	union extendedoperationmode extend_opr_mode;
    union miscellaneouscapabilities misc_cap;
	printf("UCSI MESSAGE_IN:\n");
	printf("==================\n");
	for (int i = 0; i < sizeof(*conn_cap); i++) {
		printf("%02x ", ((unsigned char*)conn_cap)[i]);
		if ((i + 1) % 8 == 0) {
			printf("\n");
		}
	}

	printf("\n\nGET_CONNECTOR_CAPABILITY:\n");
	printf("-------------------------\n");
    printf("OperationMode: 0x%x\n", conn_cap->opr_mode.raw_operationmode);
    printf("  Rponly: %d\n", conn_cap->opr_mode.rponly);
    printf("  Rdonly: %d\n", conn_cap->opr_mode.rdonly);
    printf("  Drp: %d\n", conn_cap->opr_mode.drp);
    printf("  AnalogAudioAccessorymode: %d\n", conn_cap->opr_mode.analogaudioaccessorymode);
    printf("  DebugAccessorymode: %d\n", conn_cap->opr_mode.debugaccessorymode);
    printf("  Usb2: %d\n", conn_cap->opr_mode.usb2);
    printf("  Usb3: %d\n", conn_cap->opr_mode.usb3);
    printf("  AlternateMode: %d\n", conn_cap->opr_mode.alternatemode);
    printf("Provider: %d\n", conn_cap->provider);
    printf("Consumer: %d\n", conn_cap->consumer);
    printf("SwapToDfp: %d\n", conn_cap->swap2dfp);
    printf("SwapToUfp: %d\n", conn_cap->swap2ufp);
    printf("SwapToSrc: %d\n", conn_cap->swap2src);
    printf("SwapToSnk: %d\n", conn_cap->swap2snk);
    printf("ExtendedOperationMode: 0x%x\n", conn_cap->extended_operation_mode);
    extend_opr_mode.raw_extendedoperationmode = conn_cap->extended_operation_mode;
    printf("  Usb4Gen2: %d\n", extend_opr_mode.usb4gen2);
    printf("  EprSrc: %d\n", extend_opr_mode.eprsrc);
    printf("  EprSnk: %d\n", extend_opr_mode.eprsnk);
    printf("  Usb4Gen3: %d\n", extend_opr_mode.usb4gen3);
    printf("  Usb4Gen4: %d\n", extend_opr_mode.usb4gen4);
    printf("MiscellaneousCapabilities: 0x%x\n", conn_cap->miscellaneous_capabilities);
    misc_cap.raw_miscellaneouscapabilities = conn_cap->miscellaneous_capabilities;
    printf("  FwUpdate: %d\n", misc_cap.fwupdate);
    printf("  Security: %d\n", misc_cap.security);
    printf("ReverseCurrentProtectionSupport: %d\n", conn_cap->reverse_current_protection_support);
    printf("PartnerPDRevision: %d\n", conn_cap->partner_pd_rev);
}

/**
 * Prints the USB-C Platform Policy Manager connector status data.
 *
 * @param conn_sts The structure containing the connector status data.
 */
void print_get_connector_status(struct libtypec_connector_status *conn_sts)
{
	union connectorpartnerflags conn_parn_flg;
	printf("UCSI MESSAGE_IN:\n");
	printf("==================\n");
	for (int i = 0; i < sizeof(*conn_sts); i++) {
		printf("%02x ", ((unsigned char*)conn_sts)[i]);
		if ((i + 1) % 8 == 0) {
			printf("\n");
		}
	}

	printf("\n\nUCSI_GET_CONNECTOR_STATUS:\n");
	printf("-------------------------\n");
    printf("ConnectorStatusChange: 0x%x\n", conn_sts->ConnectorStatusChange.raw_conn_stschang);
    printf("  ExternalSupplyChange: %d\n", conn_sts->ConnectorStatusChange.ExternalSupplyChange);
    printf("  Attention: %d\n", conn_sts->ConnectorStatusChange.Attention);
    printf("  SupportedProviderCapabilitiesChange: %d\n", conn_sts->ConnectorStatusChange.SupportedProviderCapabilitiesChange);
    printf("  NegotiatedPowerLevelChange: %d\n", conn_sts->ConnectorStatusChange.NegotiatedPowerLevelChange);
    printf("  PDResetComplete: %d\n", conn_sts->ConnectorStatusChange.PDResetComplete);
    printf("  SupportedCAMChange: %d\n", conn_sts->ConnectorStatusChange.SupportedCAMChange);
    printf("  BatteryChargingStatusChange: %d\n", conn_sts->ConnectorStatusChange.BatteryChargingStatusChange);
    printf("  ConnectorPartnerChanged: %d\n", conn_sts->ConnectorStatusChange.ConnectorPartnerChanged);
    printf("  PowerDirectionChanged: %d\n", conn_sts->ConnectorStatusChange.PowerDirectionChanged);
    printf("  SinkPathStatusChange: %d\n", conn_sts->ConnectorStatusChange.SinkPathStatusChange);
    printf("  ConnectChange: %d\n", conn_sts->ConnectorStatusChange.ConnectChange);
    printf("  Error: %d\n", conn_sts->ConnectorStatusChange.Error);
    printf("PowerOperationMode: %d\n", conn_sts->PowerOperationMode);
    printf("  UsbDefaultOperation: %d\n", (conn_sts->PowerOperationMode == 1) ? 1 : 0);
    printf("  BC: %d\n", (conn_sts->PowerOperationMode == 2) ? 1 : 0);
    printf("  PD: %d\n", (conn_sts->PowerOperationMode == 3) ? 1 : 0);
    printf("  UsbTypecCurrent1.5A: %d\n", (conn_sts->PowerOperationMode == 4) ? 1 : 0);
    printf("  UsbTypecCurrent3A: %d\n", (conn_sts->PowerOperationMode == 5) ? 1 : 0);
    printf("  UsbTypecCurrent5A: %d\n", (conn_sts->PowerOperationMode == 6) ? 1 : 0);
    printf("ConnectStatus: %d\n", conn_sts->ConnectStatus);
    printf("PowerDirection: %d\n", conn_sts->PowerDirection);
    printf("  Consumer: %d\n", (conn_sts->PowerDirection == 0) ? 1 : 0);
    printf("  Provider: %d\n", (conn_sts->PowerDirection == 1) ? 1 : 0);
    printf("ConnectorPartnerFlags: 0x%x\n", conn_sts->ConnectorPartnerFlags);
	conn_parn_flg.raw_conn_part_flags = conn_sts->ConnectorPartnerFlags;
    printf("  Usb: %d\n", conn_parn_flg.usb);
    printf("  Dp: %d\n", conn_parn_flg.altmode);
    printf("  Tbt: %d\n", conn_parn_flg.usb4_gen3);
    printf("  Usb4: %d\n", conn_parn_flg.usb4_gen4);
    printf("ConnectorPartnerType: %d\n", conn_sts->ConnectorPartnerType);
    printf("  DFPattached: %d\n", (conn_sts->ConnectorPartnerType == 1) ? 1 : 0);
    printf("  UFPattached: %d\n", (conn_sts->ConnectorPartnerType == 2) ? 1 : 0);
    printf("  PoweredCableNoUFPattached: %d\n", (conn_sts->ConnectorPartnerType == 3) ? 1 : 0);
    printf("  PoweredCableUFPattached: %d\n", (conn_sts->ConnectorPartnerType == 4) ? 1 : 0);
    printf("  DebugAccessoryattched: %d\n", (conn_sts->ConnectorPartnerType == 5) ? 1 : 0);
    printf("  AudioAccessoryAttached: %d\n", (conn_sts->ConnectorPartnerType == 6) ? 1 : 0);
    printf("RequestDataObject: 0x%x\n", conn_sts->RequestDataObject);
    printf("BatteryChargingCapabilityStatus: %d\n", conn_sts->BatteryChargingCapabilityStatus);
    printf("  NotCharging: %d\n", (conn_sts->BatteryChargingCapabilityStatus == 0) ? 1 : 0);
    printf("  NominalChargingRate: %d\n", (conn_sts->BatteryChargingCapabilityStatus == 1) ? 1 : 0);
    printf("  SlowChargingRate: %d\n", (conn_sts->BatteryChargingCapabilityStatus == 2) ? 1 : 0);
    printf("  VerySlowCharingRate: %d\n", (conn_sts->BatteryChargingCapabilityStatus == 3) ? 1 : 0);    
    printf("ProviderCapabilitiesLimitedReason: %d\n", conn_sts->ProviderCapabilitiesLimitedReason);
    printf("bcdPDVersionOperationMode: ");
	hex_to_decimal(conn_sts->bcdPDVersionOperationMode);
    printf("Orientation: %d\n", conn_sts->Orientation);
    printf("  DirectOrientation : %d\n", (conn_sts->Orientation == 0) ? 1 : 0);
    printf("  FlippedOrientation : %d\n", (conn_sts->Orientation == 1) ? 1 : 0);
    printf("SinkPathStatus: %d\n", conn_sts->SinkPathStatus);
    printf("ReverseCurrentProtectionStatus: %d\n", conn_sts->ReverseCurrentProtectionStatus);
    printf("PowerReadingReady: %d\n", conn_sts->PowerReadingReady);
    printf("CurrentScale: %d\n", conn_sts->CurrentScale);
    printf("PeakCurrent: %d\n", conn_sts->PeakCurrent);
    printf("AverageCurrent: %d\n", conn_sts->AverageCurrent);
    printf("VoltageScale: %d\n", conn_sts->VoltageScale);
    printf("VoltageReading: %d\n", conn_sts->VoltageReading);
    printf("Voltage: %d\n", (conn_sts->VoltageReading * conn_sts->VoltageScale * 5));
}

/**
 * Prints the USB-C Platform Policy Manager cable property data.
 *
 * @param cable_prop The structure containing the cable property data.
 */
void print_get_cable_property(struct libtypec_cable_property *cable_prop)
{
	printf("UCSI MESSAGE_IN:\n");
	printf("==================\n");
	for (int i = 0; i < sizeof(*cable_prop); i++) {
		printf("%02x ", ((unsigned char*)cable_prop)[i]);
		if ((i + 1) % 8 == 0) {
			printf("\n");
		}
	}

	printf("\n\nGET_CABLE_PROPERTY:\n");
	printf("-------------------------\n");
    printf("bmSpeedSupported: 0x%x\n", cable_prop->speed_supported);
    printf("  Bits/s: %d\n", ((cable_prop->speed_supported & 0x3) == 0) ? 1 : 0);
    printf("  Kb/s: %d\n", ((cable_prop->speed_supported & 0x3) == 1) ? 1 : 0);
    printf("  Mb/s: %d\n", ((cable_prop->speed_supported & 0x3) == 2) ? 1 : 0);
    printf("  Gb/s: %d\n", ((cable_prop->speed_supported & 0x3) == 3) ? 1 : 0);
    printf("bCurrentCapability: %d mA\n", cable_prop->current_capability * 50);
    printf("VBUSInCable: %d\n", cable_prop->vbus_support);
    printf("CableType: %d\n", cable_prop->cable_type);
    printf("  PassiveCable: %d\n", (cable_prop->cable_type == 0) ? 1 : 0);
    printf("  ActiveCable: %d\n", (cable_prop->cable_type == 1) ? 1 : 0);
    printf("Directionality: %d\n", cable_prop->directionality);
    printf("PlugEndType: %d\n", cable_prop->plug_end_type);
    printf("  USBtypeA: %d\n", (cable_prop->plug_end_type == 0) ? 1 : 0);
    printf("  USBtypeB: %d\n", (cable_prop->plug_end_type == 1) ? 1 : 0);
    printf("  USBtypeC: %d\n", (cable_prop->plug_end_type == 2) ? 1 : 0);
    printf("  Other: %d\n", (cable_prop->plug_end_type == 3) ? 1 : 0);
    printf("ModeSupport: %d\n", cable_prop->mode_support);
    printf("CablePDRevision: %d\n", cable_prop->cable_pd_revision);
    printf("Latency: %d\n", cable_prop->latency);
   
}
/**
 * Prints the USB-C Platform Policy Manager current cam data.
 *
 * @param cur_cam The structure containing the current cam data.
 */
void print_get_current_cam(struct libtypec_current_cam *cur_cam)
{
	printf("UCSI MESSAGE_IN:\n");
	printf("==================\n");
	for (int i = 0; i < sizeof(*cur_cam); i++) {
		printf("%02x ", ((unsigned char*)cur_cam)[i]);
		if ((i + 1) % 8 == 0) {
			printf("\n");
		}
	}

	printf("\n\nGET_CURRENT_CAM :\n");
	printf("-------------------------\n");
    for(int i = 0; i < 4; i++) {
        printf("CurrentAlternateMode[%d]: 0x%x\n", i, cur_cam->current_altmode[i]);
    }

}
/**
 * Prints the USB-C Platform Policy Manager connector altmodes data.
 *
 * @param
 * 	num_alt_mode number of alternate mode 
 *	alt_mode_data The structure containing the connector altmode data.
 */
void print_get_altmode(unsigned char num_alt_mode, struct altmode_data *alt_mode_data)
{
	printf("UCSI MESSAGE_IN:\n");
	printf("==================\n");
	for (int i = 0; i < sizeof(*alt_mode_data); i++) {
		printf("%02x ", ((unsigned char*)alt_mode_data)[i]);
		if ((i + 1) % 8 == 0) {
			printf("\n");
		}
	}

	printf("\n\nGET_ALTERNATE_MODE :\n");
	printf("-------------------------\n");
    for(int i = 0; i < num_alt_mode; i++) {
        printf("Mode [%d]\n", i);
        printf("  svid: 0x%x\n", alt_mode_data[i].svid);
        printf("  vdo : 0x%x\n", alt_mode_data[i].vdo);
    }

}
/**
 * Prints the USB-C Platform Policy Manager connector pdo data.
 *
 * @param
 *  num_pdo number pdos
 *  pdo_data The structure containing the connector pdo data.
 */
void print_get_pdos(unsigned char num_pdo, struct libtypec_get_pdos *pdo_data)
{
	printf("UCSI MESSAGE_IN:\n");
	printf("==================\n");
	for (int i = 0; i < sizeof(*pdo_data); i++) {
		printf("%02x ", ((unsigned char*)pdo_data)[i]);
		if ((i + 1) % 8 == 0) {
			printf("\n");
		}
	}

	printf("\n\nGET_PDOS :\n");
	printf("-------------------------\n");
    for(int i = 0; i < num_pdo; i++) {
        printf("PDO [%d]: 0x%x\n", i, pdo_data->pdo[i]);
    }

}

char *session_info[LIBTYPEC_SESSION_MAX_INDEX];

void ucsicontrol_print(const char *ucsictrl)
{
    printf("Usage: %s [OPTIONS]\n", ucsictrl);
    printf("Options:\n");
    printf("  --help, -h                            Show this help message\n");
    printf("  --conn_rst <conn_num> <soft/hard>     Reset the Connector\n");
    printf("  --get_cap                             Get Capabilities\n");
    printf("  --get_conn_cap <conn_num>             Get Connector Capability\n");
	printf("  --get_conn_sts <conn_num>             Get Connector Status\n");
	printf("  --get_cable_prop <conn_num>           Get Cable Properties\n");
	printf("  --get_cur_cam <conn_num>              Get Current Alternate mode\n");
    printf("  --get_alt_mode <conn_num> <recipient> Get Alternate Modes\n");
    printf("  --get_pdos <conn> <partner> <offset>\n"); 
    printf("             <src_snk> <type>           Get PDOS\n");
    printf("  --set_uor <conn_num> <DFP/UFP/Accept> Set USB(Data) Operation Role\n");
    printf("  --set_pdr <conn_num> <SRC/SNK/Accept> Set Power Direction Role\n");
}
int main(int argc, char *argv[])
{
    int ret;
	int option_index = 0;
    int c, conn_num;

	static const struct option long_options[] = {
    	{"help", no_argument, NULL, 'h'},
        {"conn_rst", required_argument, NULL, 't'},
    	{"get_cap", no_argument, NULL, 'g'},
		{"get_conn_cap", required_argument, NULL, 'c'},
		{"get_conn_sts", required_argument, NULL, 's'},
		{"get_cable_prop", required_argument, NULL, 'p'},
		{"get_cur_cam", required_argument, NULL, 'r'},
        {"get_alt_mode", required_argument, NULL, 'a'},
        {"get_pdos", required_argument, NULL, 'd'},
        {"set_uor", required_argument, NULL, 'o'},
        {"set_pdr", required_argument, NULL, 'e'},
    	{NULL, 0, NULL, 0} // End of options marker
	};

    // Initialize libtypec and print session info
    ret = libtypec_init(session_info, LIBTYPEC_BACKEND_DBGFS);
    if (ret < 0)
    {
        printf("Failed in Initializing libtypec\n");
        return -1;
    }

	while ((c = getopt_long(argc, argv, "htgcspradoe", long_options, &option_index)) != -1) {
        switch (c) {
            case 'h':
                ucsicontrol_print(argv[0]);
                return 0;
            case 't':
                conn_num = atoi(optarg);
                char reset_type[10];
                int rst_type;
                strcpy(reset_type, argv[optind]);
                if(strcmp(reset_type, "soft") == 0)
                    rst_type = 0;
                else if(strcmp(reset_type, "hard") == 0)
                    rst_type = 1;
                else {
                    printf("Invalid reset type: %s\n", reset_type);
                    return -1;
                }
                if (conn_num < 0) {
                    printf("Invalid connector number: %s\n", optarg);
                    return -1;
                }
                ret = libtypec_connector_reset(conn_num, rst_type);
				if(ret < 0)
					printf("connector%d %s reset failed\n", conn_num, ((rst_type == 0) ? "soft" : "hard"));
				else
					printf("connector%d %s reset\n", conn_num, ((rst_type == 0) ? "soft" : "hard"));
                return 0;
            case 'g':
                ret = libtypec_get_capability(&cap_data);
                if (ret < 0) {
                    printf("Failed in getting capabilities\n");
                    return -1;
                }
                print_get_capability(&cap_data);
                return 0;
           case 'c':
			    conn_num = atoi(optarg);
                if (conn_num < 0) {
                    printf("Invalid connector number: %s\n", optarg);
                    return -1;
                }
                ret = libtypec_get_conn_capability(conn_num, &conn_cap);
                if (ret < 0) {
                    printf("Failed in get connector capability for connector %d\n", conn_num);
                    return -1;
                }
                print_get_connector_capability(&conn_cap);
                return 0; 
            case 's':
				conn_num = atoi(optarg);
                if (conn_num < 0) {
                    printf("Invalid connector number: %s\n", optarg);
                    return -1;
                }
                ret = libtypec_get_connector_status(conn_num, &conn_sts);
                if (ret < 0) {
                    printf("Failed in get connector status for connector %d\n", conn_num);
                    return -1;
                }
                print_get_connector_status(&conn_sts);
                return 0;
            case 'p':
				conn_num = atoi(optarg);
                if (conn_num < 0) {
                    printf("Invalid connector number: %s\n", optarg);
                    return -1;
                }
                ret = libtypec_get_cable_properties(conn_num, &cable_prop);
                if (ret < 0) {
                    printf("Failed in get cable property for connector %d\n", conn_num);
                    return -1;
                }
                print_get_cable_property(&cable_prop);
                return 0;
            case 'r':
				conn_num = atoi(optarg);
                if (conn_num < 0) {
                    printf("Invalid connector number: %s\n", optarg);
                    return -1;
                }
                ret = libtypec_get_current_cam(conn_num, &cur_cam);
                if (ret < 0) {
                    printf("Failed in get current cam for connector %d\n", conn_num);
                    return -1;
                }
                print_get_current_cam(&cur_cam);
                return 0;
            case 'a':
                int num_alt_mode = 0;
                int recipient;
                conn_num = atoi(optarg);
                recipient = atoi(argv[optind]);
                if (conn_num < 0) {
                    printf("Invalid connector number: %s\n", optarg);
                    return -1;
                }
                num_alt_mode = libtypec_get_alternate_modes(recipient, conn_num, alt_mode_data);
                print_get_altmode(num_alt_mode, alt_mode_data);

                return 0;
            case 'd':
                int partner, offset, num_pdo = 0, src_snk, type;
                conn_num = atoi(optarg);
                partner = atoi(argv[3]);
                offset = atoi(argv[4]);
                src_snk = atoi(argv[5]);
                type = atoi(argv[6]); 
                
                if (conn_num < 0) {
                    printf("Invalid connector number: %s\n", optarg);
                    return -1;
                }
                ret = libtypec_get_pdos(conn_num, partner, offset, &num_pdo, src_snk, type, &pdo_data_val);
                print_get_pdos(num_pdo, &pdo_data_val);
                return 0;
            case 'o':
                conn_num = atoi(optarg);
                char uor_type[10];
                int uor;
                strcpy(uor_type, argv[optind]);
                if(strcmp(uor_type, "DFP") == 0)
                    uor = 1;
                else if(strcmp(uor_type, "UFP") == 0)
                    uor = 2;
                else if(strcmp(uor_type, "Accept") == 0)
                    uor = 4;
                else    
                    printf("Invalid UOR type: %s\n", uor_type);
                ret = libtypec_set_uor_ops(conn_num, uor);
				if (ret < 0)
					printf("connector%d set data role operation %s failed\n", conn_num, uor_type);
				else
					printf("connector%d set data role operation %s\n", conn_num, uor_type);
                return 0;
           case 'e':
                conn_num = atoi(optarg);
                char pdr_type[10];
                int pdr;
                strcpy(pdr_type, argv[optind]);
                if(strcmp(pdr_type, "SRC") == 0)
                    pdr = 1;
                else if(strcmp(pdr_type, "SNK") == 0)
                    pdr = 2;
                else if(strcmp(pdr_type, "Accept") == 0)
                    pdr = 4;
                else  
                {  
                    printf("Invalid PDR type: %s\n", pdr_type);
                    return -1;
                }
                ret = libtypec_set_pdr_ops(conn_num, pdr);
				if (ret < 0)
					printf("connector%d power role swap operation %s failed", conn_num, pdr_type);
				else
					printf("connector%d power role swap operation  %s\n", conn_num, pdr_type);
                return 0;
            default:
                ucsicontrol_print(argv[0]);
                return 0;
        }
    }

    // If no options were provided
    ucsicontrol_print(argv[0]);

   return 0;
}

