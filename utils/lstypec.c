/*
    Copyright (c) 2021-2022 by Rajaram Regupathy, rajaram.regupathy@gmail.com

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
 * @file lstypec.c
 * @author Rajaram Regupathy <rajaram.regupathy@gmail.com>
 * @coauthor Jameson Thies <jthies@google.com>
 * @brief Implements listing of typec port and port partner details
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>

#include "../libtypec.h"
#include "lstypec.h"
#include "names.h"
#include "../libtypec_config.h"

struct altmode_data am_data[64];
char *session_info[LIBTYPEC_SESSION_MAX_INDEX];

typedef struct {
    int verbose;
    int help;
    int port_num;
    int ppm;
    int partner_num;
    int cb_num;
    int am;
    int backend; // 0 for sysfs, 1 for debugfs
} CmdArgs;

CmdArgs lstypec_args;

struct option long_options[] = {
    {"verbose", no_argument, &lstypec_args.verbose, 1},
    {"help", no_argument, &lstypec_args.help, 1},
    {"[p]ort", required_argument, NULL, 'p'},
    {"ppm", no_argument, &lstypec_args.ppm, 1},
    {"partner", required_argument, NULL, 'r'},
    {"cb", required_argument, NULL, 'c'},
    {"am", no_argument, &lstypec_args.am, 1},
    {"backend", required_argument, NULL, 'b'},
    {0, 0, 0, 0}
};

void print_usage() {
    printf("-v  display verbose info\n");
    printf("-h  display usage\n");
    printf("-p [num] prints details specific of a port number indicated in num\n");
    printf("-ppm print only Platform Policy Manager details only\n");
    printf("-partner [num] print port partner details of the port represented in num\n");
    printf("-cb [num] print cable details from the particular port\n");
    printf("-am print alternate mode details of port/partner/cable\n");
    printf("-backend [string] where string is debugfs or sysfs sets, convert it to int. backend to be used by libtypec\n");
}

void parse_args(int argc, char *argv[]) {
    // Initialize the CmdArgs structure with default values
    lstypec_args.verbose = 0;
    lstypec_args.help = 0;
    lstypec_args.port_num = -1; // -1 indicates no port number specified
    lstypec_args.ppm = 0;
    lstypec_args.partner_num = -1; // -1 indicates no partner number specified
    lstypec_args.cb_num = -1; // -1 indicates no cable number specified
    lstypec_args.am = -1; // -1 indicates no alternate mode number specified
    lstypec_args.backend = 0; // default backend is sysfs

  for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0) {
            lstypec_args.verbose = 1;
        } else if (strcmp(argv[i], "-h") == 0) {
            lstypec_args.help = 1;
            print_usage();
            exit(EXIT_SUCCESS);
        } else if (strcmp(argv[i], "-p") == 0) {
            if (i+1 >= argc || !isdigit(argv[i+1][0])) {
                printf("Error: -p requires a number argument\n");
                exit(EXIT_FAILURE);
            }
            lstypec_args.port_num = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-ppm") == 0) {
            lstypec_args.ppm = 1;
        } else if (strcmp(argv[i], "-partner") == 0) {
            if (i+1 >= argc || !isdigit(argv[i+1][0])) {
                printf("Error: -partner requires a number argument\n");
                exit(EXIT_FAILURE);
            }
            lstypec_args.partner_num = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-cb") == 0) {
            if (i+1 >= argc || !isdigit(argv[i+1][0])) {
                printf("Error: -cb requires a number argument\n");
                exit(EXIT_FAILURE);
            }
            lstypec_args.cb_num = atoi(argv[++i]);
        }  else if (strcmp(argv[i], "-am") == 0) {
            lstypec_args.am = 1 ;
        } else if (strcmp(argv[i], "-backend") == 0) {
            if (i+1 >= argc || (strcmp(argv[i+1], "debugfs") != 0 && strcmp(argv[i+1], "sysfs") != 0)) {
                printf("Error: -backend requires a string argument (debugfs or sysfs)\n");
                exit(EXIT_FAILURE);
            }
            if (strcmp(argv[++i], "debugfs") == 0) {
                lstypec_args.backend = LIBTYPEC_BACKEND_DBGFS;
            } else if (strcmp(argv[i], "sysfs") == 0) {
                lstypec_args.backend = LIBTYPEC_BACKEND_SYSFS;
            }
        } else {
            printf("Error: Unknown argument %s\n", argv[i]);
            exit(EXIT_FAILURE);
        }
    }
    
}

enum product_type get_cable_product_type(short rev, uint32_t id)
{
  // Decode cable product type
  if (rev == 0x200)
  {
    // USB PD 2.0 cable
    if ((id & pd_ufp_product_type_mask) == pd2p0_passive_cable)
      return product_type_pd2p0_passive_cable;
    else if ((id & pd_ufp_product_type_mask) == pd2p0_active_cable)
      return product_type_pd2p0_active_cable;
    else
      return product_type_other;
  }
  else if (rev == 0x300)
  {
    // USB PD 3.0 cable
    if ((id & pd_ufp_product_type_mask) == pd3p0_passive_cable)
      return product_type_pd3p0_passive_cable;
    else if ((id & pd_ufp_product_type_mask) == pd3p0_active_cable)
      return product_type_pd3p0_active_cable;
    else
      return product_type_other;
  }
  else if (rev == 0x310)
  {
    // USB PD 3.1 cable
    if ((id & pd_ufp_product_type_mask) == pd3p1_passive_cable)
      return product_type_pd3p1_passive_cable;
    else if ((id & pd_ufp_product_type_mask) == pd3p1_active_cable)
      return product_type_pd3p1_active_cable;
    else if ((id & pd_ufp_product_type_mask) == pd3p1_vpd)
      return product_type_pd3p1_vpd;
    else
      return product_type_other;
  }
  return product_type_other;
}

enum product_type get_partner_product_type(short rev, uint32_t id)
{
  // Decode partner product type
  if (rev == 0x200)
  {
    // USB PD 2.0 partner
    if ((id & pd_ufp_product_type_mask) == pd2p0_ama)
      return product_type_pd2p0_ama;
    else
      return product_type_other;
  }
  else if (rev == 0x300)
  {
    // USB PD 3.0 partner
    char ufp_supported = 0;
    if ((id & pd_ufp_product_type_mask) == pd3p0_hub)
      ufp_supported = 1;
    else if ((id & pd_ufp_product_type_mask) == pd3p0_peripheral)
      ufp_supported = 1;
    else if ((id & pd_ufp_product_type_mask) == pd3p0_ama)
      return product_type_pd3p0_ama;
    else if ((id & pd_ufp_product_type_mask) == pd3p0_vpd)
      return product_type_pd3p0_vpd;

    char dfp_supported = 0;
    if ((id & pd_dfp_product_type_mask) == pd3p0_dfp_hub)
      dfp_supported = 1;
    else if ((id & pd_dfp_product_type_mask) == pd3p0_dfp_host)
      dfp_supported = 1;
    else if ((id & pd_dfp_product_type_mask) == pd3p0_power_brick)
      dfp_supported = 1;

    if (ufp_supported && dfp_supported)
      return product_type_pd3p0_drd;
    else if (ufp_supported)
      return product_type_pd3p0_ufp;
    else if (dfp_supported)
      return product_type_pd3p0_dfp;
    else
      return product_type_other;
  }
  else if (rev == 0x310)
  {
    // USB PD 3.1 partner
    char ufp_supported = 0;
    if ((id & pd_ufp_product_type_mask) == pd3p1_hub)
      ufp_supported = 1;
    else if ((id & pd_ufp_product_type_mask) == pd3p1_peripheral)
      ufp_supported = 1;

    char dfp_supported = 0;
    if ((id & pd_dfp_product_type_mask) == pd3p1_dfp_hub)
      dfp_supported = 1;
    else if ((id & pd_dfp_product_type_mask) == pd3p1_dfp_host)
      dfp_supported = 1;
    else if ((id & pd_dfp_product_type_mask) == pd3p1_power_brick)
      dfp_supported = 1;

    if (ufp_supported && dfp_supported)
      return product_type_pd3p1_drd;
    else if (ufp_supported)
      return product_type_pd3p1_ufp;
    else if (dfp_supported)
      return product_type_pd3p1_dfp;
    else
      return product_type_other;
  }
  return product_type_other;
}

void print_vdo(uint32_t vdo, int num_fields, const struct vdo_field vdo_fields[], const char *vdo_field_desc[][MAX_FIELDS])
{
  for (int i = 0; i < num_fields; i++) {
    if (!vdo_fields[i].print)
      continue;
      uint32_t field = (vdo >> vdo_fields[i].index) & vdo_fields[i].mask;
      printf("      %s: %*d", vdo_fields[i].name, FIELD_WIDTH(MAX_FIELD_LENGTH - ((int) strlen(vdo_fields[i].name))), ((vdo >> vdo_fields[i].index) & vdo_fields[i].mask));
      if (vdo_field_desc[i][0] != NULL) {
        // decode field
        printf(" (%s)\n", vdo_field_desc[i][field]);
      } else if (strcmp(vdo_fields[i].name, "USB Vendor ID")  == 0) {
        // decode vendor id
         char vendor_str[128];
         uint16_t svid = ((vdo >> vdo_fields[i].index) & vdo_fields[i].mask);
         get_vendor_string(vendor_str, sizeof(vendor_str), svid);
        printf(" (%s)\n", (vendor_str[0] == '\0' ? "unknown" : vendor_str));
      } else {
	        int unit_step;
        char unit_name[4];
        if (sscanf(vdo_fields[i].name, "%*[^0-9]%d%3s units", &unit_step, unit_name) == 2) {
          // decode unit
          printf(" (%d%s)\n", field * unit_step, unit_name);
        } else {
          // No decoding
          printf("\n");
        }
      }
  }
}

/**
 * Prints the session information for the lstypec library.
 * The session information includes the library version, operating system, kernel version, 
 * and library operations.
 */
void print_session_info()
{
  printf("lstypec %d.%d.%d Session Info\n", LIBTYPEC_MAJOR_VERSION, LIBTYPEC_MINOR_VERSION, LIBTYPEC_PATCH_VERSION);
  printf("  Using %s\n", session_info[LIBTYPEC_VERSION_INDEX]);
  printf("  %s with Kernel %s\n", session_info[LIBTYPEC_OS_INDEX], session_info[LIBTYPEC_KERNEL_INDEX]);
  printf("  libtypec using %s\n", session_info[LIBTYPEC_OPS_INDEX]);
}

/**
 * Prints the USB-C Platform Policy Manager capability data.
 *
 * @param ppm_data The structure containing the capability data.
 */
void print_ppm_capability(struct libtypec_capability_data ppm_data)
{
  printf("\nUSB-C Platform Policy Manager Capability\n");
  printf("  Number of Connectors: %x\n", ppm_data.bNumConnectors);
  printf("  Number of Alternate Modes: %x\n", ppm_data.bNumAltModes);
  printf("  USB Power Delivery Revision: %x.%x\n", (ppm_data.bcdPDVersion >> 8) & 0XFF, (ppm_data.bcdPDVersion) & 0XFF);
  printf("  USB Type-C Revision:  %x.%x\n",(ppm_data.bcdTypeCVersion >> 8) & 0XFF, (ppm_data.bcdTypeCVersion) & 0XFF);
  printf("  USB BC Revision: %x.%x\n", (ppm_data.bcdBCVersion >> 8) & 0XFF, (ppm_data.bcdBCVersion) & 0XFF);
}

void print_conn_capability(struct libtypec_connector_cap_data conn_data)
{
    char *opr_mode_str[] = {"Rp Only", "Rd Only", "DRP(Rp/Rd)", "Analog Audio", "Debug Accessory", "USB2", "USB3", "Alternate Mode"};

    printf("  Operation Modes Supported: 0x%02x ", conn_data.opr_mode.raw_operationmode);

    for (int i = 0; i < sizeof(opr_mode_str) / sizeof(opr_mode_str[0]); i++) {
      if (conn_data.opr_mode.raw_operationmode & (1 << i)) {
        printf("(%s)", opr_mode_str[i]);
      }
    }
	printf("\n");
}

void print_cable_prop(struct libtypec_cable_property cable_prop, int conn_num)
{
    char *cable_type[] = {"Passive", "Active", "Unknown"};
    char *cable_plug_type[] = {"USB Type A", "USB Type B", "USB Type C", "Non-USB Type", "Unknown"};

    printf("  Cable Property in Port %d:\n", conn_num);
    printf("    Cable Type: %s\n", cable_type[cable_prop.cable_type]);
    printf("    Cable Plug Type: %s\n", cable_plug_type[cable_prop.plug_end_type]);
}

void get_svid_string(uint32_t svid, char* str) {

    switch (svid) {
        case 0xFF01:
            strcpy(str, "Display Alternate Mode");
            break;
        case 0x8087:
            strcpy(str, "TBT Alternate Mode");
            break;
        default:
            get_vendor_string(str, sizeof(str), svid);
            break;
    }
}

void print_alternate_mode_data(int recipient, uint32_t id_header, int num_modes, struct altmode_data *am_data)
{
  char vendor_id[128];

  if (recipient == AM_CONNECTOR) {
    for (int i = 0; i < num_modes; i++) {
      get_svid_string(am_data[i].svid,vendor_id);
      printf("  Local Mode %d:\n", i);
      printf("    SVID: 0x%04x (%s)\n", am_data[i].svid,vendor_id);
      printf("    VDO: 0x%08x\n", am_data[i].vdo);
      if (lstypec_args.verbose) {
        switch(am_data[i].svid){
        case 0x8087:
          print_vdo(am_data[i].vdo, 7, tbt3_sop_fields, tbt3_sop_field_desc);
          break;
        case 0xff01:
          print_vdo(am_data[i].vdo, 7, dp_alt_mode_partner_fields, dp_alt_mode_partner_field_desc);
          break;
        default:
          get_vendor_string(vendor_id, sizeof(vendor_id), am_data[i].svid);
          printf("      VDO Decoding not supported for 0x%04x (%s)\n", am_data[i].svid, (vendor_id[0] == '\0' ? "unknown" : vendor_id));
          break;
        }
      }
    }
  }

  if (recipient == AM_SOP) {
    for (int i = 0; i < num_modes; i++) {
      get_svid_string(am_data[i].svid,vendor_id);
      printf("  Partner Mode %d:\n", i);
      printf("    SVID: 0x%04x (%s)\n", am_data[i].svid,vendor_id);
      printf("    VDO: 0x%08x\n", am_data[i].vdo);
      if (lstypec_args.verbose) {
        switch(am_data[i].svid){
        case 0x8087:
          print_vdo(am_data[i].vdo, 7, tbt3_sop_fields, tbt3_sop_field_desc);
          break;
        case 0xff01:
          print_vdo(am_data[i].vdo, 7, dp_alt_mode_partner_fields, dp_alt_mode_partner_field_desc);
          break;
        default:
          get_vendor_string(vendor_id, sizeof(vendor_id), am_data[i].svid);
          printf("      VDO Decoding not supported for 0x%04x (%s)\n", am_data[i].svid, (vendor_id[0] == '\0' ? "unknown" : vendor_id));
          break;
        }
      }
    }
  }

  if (recipient == AM_SOP_PR) {
    for (int i = 0; i < num_modes; i++) {
      get_svid_string(am_data[i].svid,vendor_id);
      printf("  Cable Plug Modes %d:\n", i);
      printf("    SVID: 0x%04x (%s)\n", am_data[i].svid,vendor_id);
      printf("    VDO: 0x%08x\n", am_data[i].vdo);

      if (lstypec_args.verbose) {
        switch(am_data[i].svid){
        case 0x8087:
          print_vdo(am_data[i].vdo, 7, tbt3_sop_pr_fields, tbt3_sop_pr_field_desc);
          break;
        case 0xff01:
          if ((id_header & ACTIVE_CABLE_MASK) == ACTIVE_CABLE_COMP) {
            print_vdo(am_data[i].vdo, 7, dp_alt_mode_active_cable_fields, dp_alt_mode_active_cable_field_desc);
          } else {
            get_vendor_string(vendor_id, sizeof(vendor_id), am_data[i].svid);
            printf("      SVID Decoding not supported for 0x%04x (%s)\n", am_data[i].svid, (vendor_id[0] == '\0' ? "unknown" : vendor_id));
          }
          break;
        default:
          get_vendor_string(vendor_id, sizeof(vendor_id), am_data[i].svid);
          printf("      SVID Decoding not supported for 0x%04x (%s)\n", am_data[i].svid, (vendor_id[0] == '\0' ? "unknown" : vendor_id));
          break;
        }
      }
    }
  }
}


void print_identity_data(int recipient, union libtypec_discovered_identity id, struct libtypec_connector_cap_data conn_data)
{
  if (recipient == AM_SOP)
  {
    printf("  Partner Identity :\n");

    // Partner ID
    if (lstypec_args.verbose)
    {
      // ID Header/Cert Stat/Product are base on revision
      switch (conn_data.partner_pd_rev)
      {
        case 0x200:
          printf("    ID Header: 0x%08x\n", id.disc_id.id_header);
          print_vdo(((uint32_t) id.disc_id.id_header), 6, pd2p0_partner_id_header_fields, pd2p0_partner_id_header_field_desc);
          printf("    Cert Stat: 0x%08x\n", id.disc_id.cert_stat);
          print_vdo(((uint32_t) id.disc_id.cert_stat), 1, pd2p0_cert_stat_fields, pd2p0_cert_stat_field_desc);
          printf("    Product: 0x%08x\n", id.disc_id.product);
          print_vdo(((uint32_t) id.disc_id.product), 2, pd2p0_product_fields, pd2p0_product_field_desc);
          break;
        case 0x300:
          printf("    ID Header: 0x%08x\n", id.disc_id.id_header);
          print_vdo(((uint32_t) id.disc_id.id_header), 7, pd3p0_partner_id_header_fields, pd3p0_partner_id_header_field_desc);
          printf("    Cert Stat: 0x%08x\n", id.disc_id.cert_stat);
          print_vdo(((uint32_t) id.disc_id.cert_stat), 1, pd3p0_cert_stat_fields, pd3p0_cert_stat_field_desc);
          printf("    Product: 0x%08x\n", id.disc_id.product);
          print_vdo(((uint32_t) id.disc_id.product), 2, pd3p0_product_fields, pd3p0_product_field_desc);
          break;
        case 0x310:
          printf("    ID Header: 0x%08x\n", id.disc_id.id_header);
          print_vdo(((uint32_t) id.disc_id.id_header), 8, pd3p1_partner_id_header_fields, pd3p1_partner_id_header_field_desc);
          printf("    Cert Stat: 0x%08x\n", id.disc_id.cert_stat);
          print_vdo(((uint32_t) id.disc_id.cert_stat), 1, pd3p1_cert_stat_fields, pd3p1_cert_stat_field_desc);
          printf("    Product: 0x%08x\n", id.disc_id.product);
          print_vdo(((uint32_t) id.disc_id.product), 2, pd3p1_product_fields, pd3p1_product_field_desc);
          break;
        default:
          printf("    ID Header: 0x%08x\n", id.disc_id.id_header);
          printf("    Cert Stat: 0x%08x\n", id.disc_id.cert_stat);
          printf("    Product: 0x%08x\n", id.disc_id.product);
          break;
      }

      //Product Type VDOs based on product type
      enum product_type partner_product_type = get_partner_product_type(conn_data.partner_pd_rev, ((uint32_t) id.disc_id.id_header));
      switch (partner_product_type)
      {
        case product_type_pd2p0_ama:
          printf("    Product VDO 1: 0x%08x\n", id.disc_id.product_type_vdo1);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo1), 11, pd2p0_ama_fields, pd2p0_ama_field_desc);
          printf("    Product VDO 2: 0x%08x\n", id.disc_id.product_type_vdo2);
          printf("    Product VDO 3: 0x%08x\n", id.disc_id.product_type_vdo3);
          break;
        case product_type_pd3p0_ama:
          printf("    Product VDO 1: 0x%08x\n", id.disc_id.product_type_vdo1);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo1), 8, pd3p0_ama_fields, pd3p0_ama_field_desc);
          printf("    Product VDO 2: 0x%08x\n", id.disc_id.product_type_vdo2);
          printf("    Product VDO 3: 0x%08x\n", id.disc_id.product_type_vdo3);
          break;
        case product_type_pd3p0_vpd:
          printf("    Product VDO 1: 0x%08x\n", id.disc_id.product_type_vdo1);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo1), 10, pd3p0_vpd_fields, pd3p0_vpd_field_desc);
          printf("    Product VDO 2: 0x%08x\n", id.disc_id.product_type_vdo2);
          printf("    Product VDO 3: 0x%08x\n", id.disc_id.product_type_vdo3);
          break;
        case product_type_pd3p0_ufp:
          printf("    Product VDO 1: 0x%08x\n", id.disc_id.product_type_vdo1);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo1), 6, pd3p0_ufp_vdo1_fields, pd3p0_ufp_vdo1_field_desc);
          printf("    Product VDO 2: 0x%08x\n", id.disc_id.product_type_vdo2);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo2), 6, pd3p0_ufp_vdo2_fields, pd3p0_ufp_vdo2_field_desc);
          printf("    Product VDO 3: 0x%08x\n", id.disc_id.product_type_vdo3);
          break;
        case product_type_pd3p0_dfp:
          printf("    Product VDO 1: 0x%08x\n", id.disc_id.product_type_vdo1);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo1), 5, pd3p0_dfp_fields, pd3p0_dfp_field_desc);
          printf("    Product VDO 2: 0x%08x\n", id.disc_id.product_type_vdo2);
          printf("    Product VDO 3: 0x%08x\n", id.disc_id.product_type_vdo3);
          break;
        case product_type_pd3p0_drd:
          printf("    Product VDO 1: 0x%08x\n", id.disc_id.product_type_vdo1);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo1), 6, pd3p0_ufp_vdo1_fields, pd3p0_ufp_vdo1_field_desc);
          printf("    Product VDO 2: 0x%08x\n", id.disc_id.product_type_vdo2);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo2), 6, pd3p0_ufp_vdo2_fields, pd3p0_ufp_vdo2_field_desc);
          printf("    Product VDO 3: 0x%08x\n", id.disc_id.product_type_vdo3);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo3), 5, pd3p0_dfp_fields, pd3p0_dfp_field_desc);
          break;
        case product_type_pd3p1_ufp:
          printf("    Product VDO 1: 0x%08x\n", id.disc_id.product_type_vdo1);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo1), 10, pd3p1_ufp_fields, pd3p1_ufp_field_desc);
          printf("    Product VDO 2: 0x%08x\n", id.disc_id.product_type_vdo2);
          printf("    Product VDO 3: 0x%08x\n", id.disc_id.product_type_vdo3);
          break;
        case product_type_pd3p1_dfp:
          printf("    Product VDO 1: 0x%08x\n", id.disc_id.product_type_vdo1);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo1), 6, pd3p1_dfp_fields, pd3p1_dfp_field_desc);
          printf("    Product VDO 2: 0x%08x\n", id.disc_id.product_type_vdo2);
          printf("    Product VDO 3: 0x%08x\n", id.disc_id.product_type_vdo3);
          break;
        case product_type_pd3p1_drd:
          printf("    Product VDO 1: 0x%08x\n", id.disc_id.product_type_vdo1);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo1), 10, pd3p1_ufp_fields, pd3p1_ufp_field_desc);
          printf("    Product VDO 2: 0x%08x\n", id.disc_id.product_type_vdo2);
          printf("    Product VDO 3: 0x%08x\n", id.disc_id.product_type_vdo3);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo3), 6, pd3p1_dfp_fields, pd3p1_dfp_field_desc);
          break;
        default:
          printf("    Product VDO 1: 0x%08x\n", id.disc_id.product_type_vdo1);
          printf("    Product VDO 2: 0x%08x\n", id.disc_id.product_type_vdo2);
          printf("    Product VDO 3: 0x%08x\n", id.disc_id.product_type_vdo3);
          break;
      }
    }
    else
    {
      printf("    ID Header: 0x%08x\n", id.disc_id.id_header);
      printf("    Cert Stat: 0x%08x\n", id.disc_id.cert_stat);
      printf("    Product: 0x%08x\n", id.disc_id.product);
      printf("    Product VDO 1: 0x%08x\n", id.disc_id.product_type_vdo1);
      printf("    Product VDO 2: 0x%08x\n", id.disc_id.product_type_vdo2);
      printf("    Product VDO 3: 0x%08x\n", id.disc_id.product_type_vdo3);
    }
  }
  else if (recipient == AM_SOP_PR)
  {

    printf("  Cable Identity :\n");

    // Partner ID
    if (lstypec_args.verbose)
    {
      // ID Header/Cert Stat/Product are base on revision
      switch (conn_data.partner_pd_rev)
      {
        case 0x200:
          printf("    ID Header: 0x%08x\n", id.disc_id.id_header);
          print_vdo(((uint32_t) id.disc_id.id_header), 6, pd2p0_cable_id_header_fields, pd2p0_cable_id_header_field_desc);
          printf("    Cert Stat: 0x%08x\n", id.disc_id.cert_stat);
          print_vdo(((uint32_t) id.disc_id.cert_stat), 1, pd2p0_cert_stat_fields, pd2p0_cert_stat_field_desc);
          printf("    Product: 0x%08x\n", id.disc_id.product);
          print_vdo(((uint32_t) id.disc_id.product), 2, pd2p0_product_fields, pd2p0_product_field_desc);
          break;
        case 0x300:
          printf("    ID Header: 0x%08x\n", id.disc_id.id_header);
          print_vdo(((uint32_t) id.disc_id.id_header), 7, pd3p0_cable_id_header_fields, pd3p0_cable_id_header_field_desc);
          printf("    Cert Stat: 0x%08x\n", id.disc_id.cert_stat);
          print_vdo(((uint32_t) id.disc_id.cert_stat), 1, pd3p0_cert_stat_fields, pd3p0_cert_stat_field_desc);
          printf("    Product: 0x%08x\n", id.disc_id.product);
          print_vdo(((uint32_t) id.disc_id.product), 2, pd3p0_product_fields, pd3p0_product_field_desc);
          break;
        case 0x310:
          printf("    ID Header: 0x%08x\n", id.disc_id.id_header);
          print_vdo(((uint32_t) id.disc_id.id_header), 8, pd3p1_cable_id_header_fields, pd3p1_cable_id_header_field_desc);
          printf("    Cert Stat: 0x%08x\n", id.disc_id.cert_stat);
          print_vdo(((uint32_t) id.disc_id.cert_stat), 1, pd3p1_cert_stat_fields, pd3p1_cert_stat_field_desc);
          printf("    Product: 0x%08x\n", id.disc_id.product);
          print_vdo(((uint32_t) id.disc_id.product), 2, pd3p1_product_fields, pd3p1_product_field_desc);
          break;
        default:
          printf("    ID Header: 0x%08x\n", id.disc_id.id_header);
          printf("    Cert Stat: 0x%08x\n", id.disc_id.cert_stat);
          printf("    Product: 0x%08x\n", id.disc_id.product);
          break;
      }

      //Product Type VDOs based on product type
      enum product_type cable_product_type = get_cable_product_type(conn_data.partner_pd_rev, ((uint32_t) id.disc_id.id_header));
      switch (cable_product_type)
      {
        case product_type_pd2p0_passive_cable:
          printf("    Product VDO 1: 0x%08x\n", id.disc_id.product_type_vdo1);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo1), 15, pd2p0_passive_cable_fields, pd2p0_passive_cable_field_desc);
          printf("    Product VDO 2: 0x%08x\n", id.disc_id.product_type_vdo2);
          printf("    Product VDO 3: 0x%08x\n", id.disc_id.product_type_vdo3);
          break;
        case product_type_pd2p0_active_cable:
          printf("    Product VDO 1: 0x%08x\n", id.disc_id.product_type_vdo1);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo1), 15, pd2p0_active_cable_fields, pd2p0_active_cable_field_desc);
          printf("    Product VDO 2: 0x%08x\n", id.disc_id.product_type_vdo2);
          printf("    Product VDO 3: 0x%08x\n", id.disc_id.product_type_vdo3);
          break;
        case product_type_pd3p0_passive_cable:
          printf("    Product VDO 1: 0x%08x\n", id.disc_id.product_type_vdo1);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo1), 13, pd3p0_passive_cable_fields, pd3p0_passive_cable_field_desc);
          printf("    Product VDO 2: 0x%08x\n", id.disc_id.product_type_vdo2);
          printf("    Product VDO 3: 0x%08x\n", id.disc_id.product_type_vdo3);
          break;
        case product_type_pd3p0_active_cable:
          printf("   Product VDO 1: 0x%08x\n", id.disc_id.product_type_vdo1);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo1), 15, pd3p0_active_cable_vdo1_fields, pd3p0_active_cable_vdo1_field_desc);
          printf("    Product VDO 2: 0x%08x\n", id.disc_id.product_type_vdo2);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo2), 15, pd3p0_active_cable_vdo2_fields, pd3p0_active_cable_vdo2_field_desc);
          printf("    Product VDO 3: 0x%08x\n", id.disc_id.product_type_vdo3);
          break;
        case product_type_pd3p1_passive_cable:
          printf("    Product VDO 1: 0x%08x\n", id.disc_id.product_type_vdo1);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo1), 13, pd3p1_passive_cable_fields, pd3p1_passive_cable_field_desc);
          printf("    Product VDO 2: 0x%08x\n", id.disc_id.product_type_vdo2);
          printf("    Product VDO 3: 0x%08x\n", id.disc_id.product_type_vdo3);
          break;
        case product_type_pd3p1_active_cable:
          printf("    Product VDO 1: 0x%08x\n", id.disc_id.product_type_vdo1);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo1), 15, pd3p1_active_cable_vdo1_fields, pd3p1_active_cable_vdo1_field_desc);
          printf("    Product VDO 2: 0x%08x\n", id.disc_id.product_type_vdo2);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo2), 15, pd3p1_active_cable_vdo2_fields, pd3p1_active_cable_vdo2_field_desc);
          printf("    Product VDO 3: 0x%08x\n", id.disc_id.product_type_vdo3);
          break;
        case product_type_pd3p1_vpd:
          printf("    Product VDO 1: 0x%08x\n", id.disc_id.product_type_vdo1);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo1), 10, pd3p1_vpd_fields, pd3p0_vpd_field_desc);
          printf("    Product VDO 2: 0x%08x\n", id.disc_id.product_type_vdo2);
          printf("    Product VDO 3: 0x%08x\n", id.disc_id.product_type_vdo3);
          break;
        default:
          printf("    Product VDO 1: 0x%08x\n", id.disc_id.product_type_vdo1);
          printf("    Product VDO 2: 0x%08x\n", id.disc_id.product_type_vdo2);
          printf("    Product VDO 3: 0x%08x\n", id.disc_id.product_type_vdo3);
          break;
      }
    }
    else
    {
      printf("    ID Header: 0x%08x\n", id.disc_id.id_header);
      printf("    Cert Stat: 0x%08x\n", id.disc_id.cert_stat);
      printf("    Product: 0x%08x\n", id.disc_id.product);
      printf("    Product VDO 1: 0x%08x\n", id.disc_id.product_type_vdo1);
      printf("    Product VDO 2: 0x%08x\n", id.disc_id.product_type_vdo2);
      printf("    Product VDO 3: 0x%08x\n", id.disc_id.product_type_vdo3);
    }
  }
}

void print_source_pdo_data(struct libtypec_get_pdos *pdo_data, int num_pdos, int revision) {
  for (int i = 0; i < num_pdos; i++) {
    printf("    PDO%d: 0x%08x\n", i+1, pdo_data->pdo[i]);

    if (lstypec_args.verbose) {
      if (revision == 0x200) {
        switch((pdo_data->pdo[i] >> 30)) {
          case PDO_FIXED:
            print_vdo(pdo_data->pdo[i], 10, pd2p0_fixed_supply_src_fields, pd2p0_fixed_supply_src_field_desc);
            break;
          case PDO_BATTERY:
            print_vdo(pdo_data->pdo[i], 4, pd2p0_battery_supply_src_fields, pd2p0_battery_supply_src_field_desc);
            break;
          case PDO_VARIABLE:
            print_vdo(pdo_data->pdo[i], 4, pd2p0_variable_supply_src_fields, pd2p0_variable_supply_src_field_desc);
            break;
        }
      } else if (revision == 0x300) {
        switch((pdo_data->pdo[i] >> 30)) {
          case PDO_FIXED:
            print_vdo(pdo_data->pdo[i], 11, pd3p0_fixed_supply_src_fields, pd3p0_fixed_supply_src_field_desc);
            break;
          case PDO_BATTERY:
            print_vdo(pdo_data->pdo[i], 4, pd3p0_battery_supply_src_fields, pd3p0_battery_supply_src_field_desc);
            break;
          case PDO_VARIABLE:
            print_vdo(pdo_data->pdo[i], 4, pd3p0_variable_supply_src_fields, pd3p0_variable_supply_src_field_desc);
            break;
          case PDO_AUGMENTED:
            print_vdo(pdo_data->pdo[i], 9, pd3p0_pps_apdo_src_fields, pd3p0_pps_apdo_src_field_desc);
            break;
        }
      } else if (revision == 0x310) {
        switch((pdo_data->pdo[i] >> 30)) {
          case PDO_FIXED:
            print_vdo(pdo_data->pdo[i], 12, pd3p1_fixed_supply_src_fields, pd3p1_fixed_supply_src_field_desc);
            break;
          case PDO_BATTERY:
            print_vdo(pdo_data->pdo[i], 4, pd3p1_battery_supply_src_fields, pd3p1_battery_supply_src_field_desc);
            break;
          case PDO_VARIABLE:
            print_vdo(pdo_data->pdo[i], 4, pd3p1_variable_supply_src_fields, pd3p1_variable_supply_src_field_desc);
            break;
          case PDO_AUGMENTED:
            print_vdo(pdo_data->pdo[i], 9, pd3p1_pps_apdo_src_fields, pd3p1_pps_apdo_src_field_desc);
            break;
        }
      }
    }
  }
}

void print_sink_pdo_data(struct libtypec_get_pdos *pdo_data, int num_pdos, int revision) {
  for (int i = 0; i < num_pdos; i++) {
    printf("    PDO%d: 0x%08x\n", i+1, pdo_data->pdo[i]);

    if (lstypec_args.verbose) {
      if (revision == 0x200) {
        switch((pdo_data->pdo[i] >> 30)) {
          case PDO_FIXED:
            print_vdo(pdo_data->pdo[i], 9, pd2p0_fixed_supply_snk_fields, pd2p0_fixed_supply_snk_field_desc);
            break;
          case PDO_BATTERY:
            print_vdo(pdo_data->pdo[i], 4, pd2p0_battery_supply_snk_fields, pd2p0_battery_supply_snk_field_desc);
            break;
          case PDO_VARIABLE:
            print_vdo(pdo_data->pdo[i], 4, pd2p0_variable_supply_snk_fields, pd2p0_variable_supply_snk_field_desc);
            break;
        }
      } else if (revision == 0x300) {
        switch((pdo_data->pdo[i] >> 30)) {
          case PDO_FIXED:
            print_vdo(pdo_data->pdo[i], 10, pd3p0_fixed_supply_snk_fields, pd3p0_fixed_supply_snk_field_desc);
            break;
          case PDO_BATTERY:
            print_vdo(pdo_data->pdo[i], 4, pd3p0_battery_supply_snk_fields, pd3p0_battery_supply_snk_field_desc);
            break;
          case PDO_VARIABLE:
            print_vdo(pdo_data->pdo[i], 4, pd3p0_variable_supply_snk_fields, pd3p0_variable_supply_snk_field_desc);
            break;
          case PDO_AUGMENTED:
            print_vdo(pdo_data->pdo[i], 8, pd3p0_pps_apdo_snk_fields, pd3p0_pps_apdo_snk_field_desc);
            break;
        }
      } else if (revision == 0x310) {
        switch((pdo_data->pdo[i] >> 30)) {
          case PDO_FIXED:
            print_vdo(pdo_data->pdo[i], 10, pd3p1_fixed_supply_snk_fields, pd3p1_fixed_supply_snk_field_desc);
            break;
          case PDO_BATTERY:
            print_vdo(pdo_data->pdo[i], 4, pd3p1_battery_supply_snk_fields, pd3p1_battery_supply_snk_field_desc);
            break;
          case PDO_VARIABLE:
            print_vdo(pdo_data->pdo[i], 4, pd3p1_variable_supply_snk_fields, pd3p1_variable_supply_snk_field_desc);
            break;
          case PDO_AUGMENTED:
            print_vdo(pdo_data->pdo[i], 8, pd3p1_pps_apdo_snk_fields, pd3p1_pps_apdo_snk_field_desc);
            break;
        }
      }
    }
  }
}

void lstypec_print(char *val, int type)
{
    if (type == LSTYPEC_ERROR)
    {
        printf("lstypec - ERROR - %s\n", val);
        exit(1);
    }
    else
        printf("lstypec - INFO - %s\n", val);
}
void print_capabilities_partner(int i)
{
    int ret, num_modes, num_pdos;

   // Partner
    num_modes = libtypec_get_alternate_modes(AM_SOP, i, am_data);
    if (num_modes >= 0) 
      print_alternate_mode_data(AM_SOP, id.disc_id.id_header, num_modes, am_data);
    ret = libtypec_get_pd_message(AM_SOP, i, 24, DISCOVER_ID_REQ, id.buf_disc_id);
    if (ret >= 0) {
      print_identity_data(AM_SOP, id, conn_data);
    }
    pdo_data = malloc(sizeof(int)*8);
    
    ret = libtypec_get_pdos(i, 1, 0, &num_pdos, 1, 0, pdo_data);
    if (ret > 0) {
      printf("  Partner PDO Data (Source):\n");
      print_source_pdo_data(pdo_data, num_pdos, conn_data.partner_pd_rev);
    }
   
    ret = libtypec_get_pdos(i, 1, 0, &num_pdos, 0, 0, pdo_data);
    if (ret > 0) {
      printf("  Partner PDO Data (Sink):\n");
      print_sink_pdo_data(pdo_data, num_pdos, conn_data.partner_pd_rev);
    }
  
    free(pdo_data);

}
void print_capabilities_cable(int i)
{
    int ret, num_modes;

     // Resetting port properties
    cable_prop.cable_type = CABLE_TYPE_PASSIVE;
    cable_prop.plug_end_type = PLUG_TYPE_OTH;
   // Cable Properties
    
    ret = libtypec_get_cable_properties(i, &cable_prop);
    if (ret >= 0)
      print_cable_prop(cable_prop, i);

        // Cable
    num_modes = libtypec_get_alternate_modes(AM_SOP_PR, i, am_data);
    if (num_modes >= 0) 
      print_alternate_mode_data(AM_SOP_PR, id.disc_id.id_header, num_modes, am_data);

    ret = libtypec_get_pd_message(AM_SOP_PR, i, 24, DISCOVER_ID_REQ, id.buf_disc_id);
    if (ret >= 0) {
      print_identity_data(AM_SOP_PR, id, conn_data);
    }

}
void print_capabilities_port(int i)
{
    int ret, num_modes, num_pdos;

    // Connector Capabilities
	printf("\nConnector %d Capability/Status\n", i);
    libtypec_get_conn_capability(i, &conn_data);
    print_conn_capability(conn_data);

    // Connector PDOs
    pdo_data = malloc(sizeof(int)*8);
    ret = libtypec_get_pdos(i, 0, 0, &num_pdos, 1, 0, pdo_data);
    if (ret > 0) {
      printf("  Connector PDO Data (Source):\n");
      print_source_pdo_data(pdo_data, num_pdos, get_cap_data.bcdPDVersion);
    }
    else
        printf("  Connector PDO Data (Source) returned : %d\n", ret);

    ret = libtypec_get_pdos(i, 0, 0, &num_pdos, 0, 0, pdo_data);
    if (ret > 0) {
      printf("  Connector PDO Data (Sink):\n");
      print_sink_pdo_data(pdo_data, num_pdos, get_cap_data.bcdPDVersion);
    }
    else
        printf("  Connector PDO Data (Source) returned : %d\n", ret);

    free(pdo_data);
    
    // Supported Alternate Modes
    printf("  Alternate Modes Supported:\n");
  
    num_modes = libtypec_get_alternate_modes(AM_CONNECTOR, i, am_data);
    if (num_modes > 0)
      print_alternate_mode_data(AM_CONNECTOR, 0x0, num_modes, am_data);
    else
      printf("    No Local Modes listed with typec class\n");
   
}
void lstypec_print_am()
{
  int ret;

  if(lstypec_args.backend == LIBTYPEC_BACKEND_DBGFS)
    ret = libtypec_init(session_info,LIBTYPEC_BACKEND_DBGFS);
  else
    // Initialize libtypec by default with sysfs and print session info
    ret = libtypec_init(session_info,LIBTYPEC_BACKEND_SYSFS);

  if (ret < 0)
    lstypec_print("Failed in Initializing libtypec", LSTYPEC_ERROR);

  // PPM Capabilities
  ret = libtypec_get_capability(&get_cap_data);
  if (ret < 0)
    lstypec_print("Failed in Get Capability", LSTYPEC_ERROR);

  if(lstypec_args.port_num != -1)
  {
      if(lstypec_args.port_num >= get_cap_data.bNumConnectors) 
      {
          printf("lstypec - ERROR - %s, Provide one less than Num Ports %d \n", "Port number out of range",get_cap_data.bNumConnectors);
          exit(1);
      }
      int num_modes = libtypec_get_alternate_modes(AM_CONNECTOR, lstypec_args.port_num, am_data);

      if (num_modes > 0)
      print_alternate_mode_data(AM_CONNECTOR, 0x0, num_modes, am_data);
      else
      printf("    No Local Modes listed with typec class\n"); 
  }
  else if(lstypec_args.partner_num != -1)
  {
    if(lstypec_args.partner_num >= get_cap_data.bNumConnectors) 
    {
        printf("lstypec - ERROR - %s, Provide one less than Num Ports %d \n", "Port number out of range",get_cap_data.bNumConnectors);
        exit(1);
    }
     // Partner
    int num_modes = libtypec_get_alternate_modes(AM_SOP, lstypec_args.partner_num, am_data);
    if (num_modes >= 0) 
      print_alternate_mode_data(AM_SOP, id.disc_id.id_header, num_modes, am_data);
    ret = libtypec_get_pd_message(AM_SOP, lstypec_args.partner_num, 24, DISCOVER_ID_REQ, id.buf_disc_id);
    if (ret >= 0) {
      print_identity_data(AM_SOP, id, conn_data);
    }
  }
  else if(lstypec_args.cb_num != -1)
  {
    if(lstypec_args.cb_num >= get_cap_data.bNumConnectors) 
    {
        printf("lstypec - ERROR - %s, Provide one less than Num Ports %d \n", "Port number out of range",get_cap_data.bNumConnectors);
        exit(1);
    }
            // Cable
    int num_modes = libtypec_get_alternate_modes(AM_SOP_PR, lstypec_args.cb_num, am_data);
    if (num_modes >= 0) 
      print_alternate_mode_data(AM_SOP_PR, id.disc_id.id_header, num_modes, am_data);

  }
  else
  {
    printf("lstypec - ERROR - %s\n", "Provide Port/Partner/Cable number for Alternate Mode Information");
    exit(1);
  }

  printf("\n");
}
void lstypec_print_partner()
{
  int ret;

  if(lstypec_args.backend == LIBTYPEC_BACKEND_DBGFS)
    ret = libtypec_init(session_info,LIBTYPEC_BACKEND_DBGFS);
  else
    // Initialize libtypec by default with sysfs and print session info
    ret = libtypec_init(session_info,LIBTYPEC_BACKEND_SYSFS);

  if (ret < 0)
    lstypec_print("Failed in Initializing libtypec", LSTYPEC_ERROR);

  // PPM Capabilities
  ret = libtypec_get_capability(&get_cap_data);
  if (ret < 0)
    lstypec_print("Failed in Get Capability", LSTYPEC_ERROR);

  if(lstypec_args.partner_num >= get_cap_data.bNumConnectors) 
  {
        printf("lstypec - ERROR - %s, Provide one less than Num Ports %d \n", "Port number out of range",get_cap_data.bNumConnectors);
        exit(1);
  }
  
  print_capabilities_partner(lstypec_args.partner_num);

}
void lstypec_print_cable()
{
  int ret;

  if(lstypec_args.backend == LIBTYPEC_BACKEND_DBGFS)
    ret = libtypec_init(session_info,LIBTYPEC_BACKEND_DBGFS);
  else
    // Initialize libtypec by default with sysfs and print session info
    ret = libtypec_init(session_info,LIBTYPEC_BACKEND_SYSFS);

  if (ret < 0)
    lstypec_print("Failed in Initializing libtypec", LSTYPEC_ERROR);

  // PPM Capabilities
  ret = libtypec_get_capability(&get_cap_data);
  if (ret < 0)
    lstypec_print("Failed in Get Capability", LSTYPEC_ERROR);

  if(lstypec_args.cb_num >= get_cap_data.bNumConnectors) 
  {
        printf("lstypec - ERROR - %s, Provide one less than Num Ports %d \n", "Port number out of range",get_cap_data.bNumConnectors);
        exit(1);
  }
  print_capabilities_cable(lstypec_args.cb_num);

}

void lstypec_print_port()
{
  int ret;

  if(lstypec_args.backend == LIBTYPEC_BACKEND_DBGFS)
    ret = libtypec_init(session_info,LIBTYPEC_BACKEND_DBGFS);
  else
    // Initialize libtypec by default with sysfs and print session info
    ret = libtypec_init(session_info,LIBTYPEC_BACKEND_SYSFS);

  if (ret < 0)
    lstypec_print("Failed in Initializing libtypec", LSTYPEC_ERROR);

  // PPM Capabilities
  ret = libtypec_get_capability(&get_cap_data);
  if (ret < 0)
    lstypec_print("Failed in Get Capability", LSTYPEC_ERROR);

  if(lstypec_args.port_num >= get_cap_data.bNumConnectors) 
  {
        printf("lstypec - ERROR - %s, Provide one less than Num Ports %d \n", "Port number out of range",get_cap_data.bNumConnectors);
        exit(1);
  }
  
  print_capabilities_port(lstypec_args.port_num);

}

void lstypec_default_verbose()
{
  int ret;

  if(lstypec_args.backend == LIBTYPEC_BACKEND_DBGFS)
    ret = libtypec_init(session_info,LIBTYPEC_BACKEND_DBGFS);
  else
    // Initialize libtypec by default with sysfs and print session info
    ret = libtypec_init(session_info,LIBTYPEC_BACKEND_SYSFS);

  if (ret < 0)
    lstypec_print("Failed in Initializing libtypec", LSTYPEC_ERROR);

  print_session_info();

  // PPM Capabilities
  ret = libtypec_get_capability(&get_cap_data);
  if (ret < 0)
    lstypec_print("Failed in Get Capability", LSTYPEC_ERROR);

  print_ppm_capability(get_cap_data);
  for (int i = 0; i < get_cap_data.bNumConnectors; i++) 
  {
      print_capabilities_port(i);
      print_capabilities_cable(i);
      print_capabilities_partner(i);
  }

  printf("\n");

}
int main(int argc, char *argv[])
{

  parse_args(argc, argv);

  names_init();

  if (lstypec_args.port_num != -1) {
      lstypec_print_port();
      goto cleanup;
  }

    if (lstypec_args.ppm) 
    {
      int ret;

      if(lstypec_args.backend == LIBTYPEC_BACKEND_DBGFS)
        ret = libtypec_init(session_info,LIBTYPEC_BACKEND_DBGFS);
      else
        // Initialize libtypec by default with sysfs and print session info
        ret = libtypec_init(session_info,LIBTYPEC_BACKEND_SYSFS);

      if (ret < 0)
        lstypec_print("Failed in Initializing libtypec", LSTYPEC_ERROR);

      print_session_info();

      // PPM Capabilities
      ret = libtypec_get_capability(&get_cap_data);
      if (ret < 0)
        lstypec_print("Failed in Get Capability", LSTYPEC_ERROR);

      print_ppm_capability(get_cap_data);

      goto cleanup;
    }
    if(lstypec_args.am != -1)
    {
        lstypec_print_am();
        goto cleanup;
    }
    if (lstypec_args.partner_num != -1) {
        lstypec_print_partner();
        goto cleanup;
    }

    if (lstypec_args.cb_num != -1) {
        lstypec_print_cable();
        goto cleanup;
    }

  lstypec_default_verbose();
cleanup:
  names_exit();
}

