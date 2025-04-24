
/*
MIT License

Copyright (c) 2023 Rajaram Regupathy <rajaram.regupathy@gmail.com>

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
 * @file libtypec_dbgfs_ops.c
 * @author Rajaram Regupathy <rajaram.regupathy@gmail.com>
 * @brief Functions for libtypec debugfs based operations
 */

#include "libtypec_ops.h"
#include <dirent.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <ftw.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>

int fp_command;
int fp_response;
struct pollfd  pfds;

int hexCharToInt(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1; // Invalid character
}
int get_ucsi_response(unsigned char *data) {
    char c[64];
    unsigned char temp[64]; // Temporary buffer for reversal, assuming data will not exceed 64 bytes
    int i = 0, j = 0, result, dataIndex = 0;

    if (fp_response <= 0) return -1;

    result = poll(&pfds, 1, -1);
    if (result < 0) return -1;

    j = read(fp_response, c, 64);
    if (j <= 2) return -1; // Not enough data read or no data to process

    // Process two characters at a time
    for (i = 2; i < j - 1; i += 2) { // Ensure there are always pairs of characters to process
        int high = hexCharToInt(c[i]);
        int low = hexCharToInt(c[i + 1]);
        if (high < 0 || low < 0) return -1; // Invalid hex character

        temp[dataIndex] = (high << 4) | low; // Combine two hex digits into one byte, store in temp
        dataIndex++;
    }

    // Reverse the bytes from temp into data
    for (i = 0; i < dataIndex; i++) {
        data[i] = temp[dataIndex - 1 - i];
    }
    lseek(fp_response, 0, SEEK_SET);
    return dataIndex; // Return the number of bytes processed and stored in data
}
#define MAX_PATH 1000

int search_dbgfs_files(char *basePath, char *commandPath, char *responsePath) {
    char path[MAX_PATH];
    struct dirent *dp;
    DIR *dir = opendir(basePath);
    int commandFound = 0, responseFound = 0;

    // Unable to open directory stream
    if (!dir)
        return -1;

    while ((dp = readdir(dir)) != NULL) {
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0) {
            // Construct new path from our base path
            strncpy(path, basePath, sizeof(path));
            strncat(path, "/", sizeof(path) - strlen(path) - 1);
            strncat(path, dp->d_name, sizeof(path) - strlen(path) - 1);

            if (strcmp(dp->d_name, "command") == 0) {
                strncpy(commandPath, path, MAX_PATH);
                commandFound = 1;
            }

            if (strcmp(dp->d_name, "response") == 0) {
                strncpy(responsePath, path, MAX_PATH);
                responseFound = 1;
            }

            if (commandFound && responseFound) {
                closedir(dir);
                return 0;
            }

            if (search_dbgfs_files(path, commandPath, responsePath) == 0) {
                closedir(dir);
                return 0;
            }
        }
    }

    closedir(dir);
    return -1;
}
static int libtypec_dbgfs_init(char **session_info)
{

 	char commandPath[MAX_PATH] = {0};
    char responsePath[MAX_PATH] = {0};

    if (search_dbgfs_files("/sys/kernel/debug/usb/ucsi", commandPath, responsePath) == 0) 
	{
		fp_command = open(commandPath, O_WRONLY);
		
		if (fp_command <= 0)
			return -1;

		fp_response = open(responsePath,O_RDONLY);
		
		if (fp_response <= 0)
			return -1;

		pfds.fd = fp_response;
		pfds.events = POLLIN;
		
		return 0;
	}
	else
	{
		printf("Failed to open ucsi debugfs files\n");

		return -EIO;
	}
}
static int libtypec_dbgfs_connector_reset_ops(int conn_num, int rst_type)
{
	int ret=-1;
	unsigned char buf[64];
	union conn_rst_cmd
	{
		struct {
			unsigned int cmd : 8;
			unsigned int data_leng : 8;
			unsigned int con_num : 7;
			unsigned int rst_type : 1;
		};
		unsigned int rst_cmd;
	}rstcmd;

	if(fp_command > 0)
	{
		rstcmd.cmd = 0x03;
		rstcmd.data_leng = 0x00;
		rstcmd.con_num = conn_num + 1;
		rstcmd.rst_type = rst_type;

		snprintf(buf, sizeof(buf), "0x%x", rstcmd.rst_cmd);
		ret = write(fp_command,buf,sizeof(buf));

		if(ret)
		{
			ret = get_ucsi_response(buf);
			if(ret < 16)
				ret = -1;
		}
	}
	return ret;
}

static int libtypec_dbgfs_exit(void)
{
	close(fp_command);
	close(fp_response);
    fp_command = -1;
    fp_response = -1;
	return 0;
}

static int libtypec_dbgfs_get_capability_ops(struct libtypec_capability_data *cap_data)
{
	int ret=-1;
	char buf[64] = {0};

	if(fp_command > 0)
	{
		ret = write(fp_command,"6",sizeof("6"));
 
		if(ret)
		{
			ret = get_ucsi_response(buf);
			if(ret < 16)
				ret = -1;

			// Copy the entire buf into cap_data
			memcpy(cap_data, buf, sizeof(*cap_data));
		}
	}
	return ret;
}

static int libtypec_dbgfs_get_conn_capability_ops(int conn_num, struct libtypec_connector_cap_data *conn_cap_data)
{
	int ret=-1;
	unsigned char buf[64] = {0};

    if(fp_command > 0)
	{
		snprintf(buf, sizeof(buf), "0x%x", (conn_num + 1) << 16 | 0x7);
		ret = write(fp_command,buf,sizeof(buf));

		if(ret)
		{
			ret = get_ucsi_response(buf);
			if(ret < 16)
				ret = -1;
			// Copy the entire buf into cap_data
			memcpy(conn_cap_data, buf, sizeof(*conn_cap_data));
		}
	}
	return ret;
}
static int libtypec_dbgfs_get_alternate_modes(int recipient, int conn_num, struct altmode_data *alt_mode_data)
{

	union get_am_cmd
	{
		unsigned long long cmd_val;
		struct{
			char cmd;
			char len;
			char rcp;
			char con;
			char offset;
			char num_am;
		}s;
	}am_cmd;
	int ret=-1,i=0;
	unsigned char buf[64];
	unsigned short psvid = 0;

	if(fp_command > 0)
	{
		do
		{
			am_cmd.s.cmd = 0xc;
			am_cmd.s.len = 0;
			am_cmd.s.rcp = recipient;
			am_cmd.s.con = conn_num+1;
			am_cmd.s.offset = i;
			am_cmd.s.num_am = 0;
			snprintf(buf, sizeof(buf), "%lld", am_cmd.cmd_val);
			ret = write(fp_command,buf,sizeof(buf));
			if(ret)
			{
				ret = get_ucsi_response(buf);
				if(ret< 16)
					return -1;
				
				alt_mode_data[i].svid 	 = buf[1] << 8 | buf[0];
				alt_mode_data[i].vdo 	 = buf[5] << 24 | buf[4] << 16 | buf[3] << 8 | buf[2];

				if((alt_mode_data[i].svid == 0) | (alt_mode_data[i].svid == psvid))
					break;
				psvid = alt_mode_data[i].svid;
			}
			i++;

		}while(1);

	}
	return i;
}
static int libtypec_dbfs_get_current_cam_ops(int conn_num, struct libtypec_current_cam *cur_cam)
{
	int ret=-1;
	unsigned char buf[64];

	if(fp_command > 0)
	{
		snprintf(buf, sizeof(buf), "0x%x", (conn_num + 1) << 16 | 0x0E);
		ret = write(fp_command,buf,sizeof(buf));
		if(ret)
		{
			ret = get_ucsi_response(buf);
			if(ret < 16)
				ret = -1;
			// Copy the entire buf into cap_data
			memcpy(cur_cam, buf, ret);
		}
	}
    return ret;
}
static int libtypec_dbgfs_get_pdos_ops(int conn_num, int partner, int offset, int *num_pdo, int src_snk, int type, struct libtypec_get_pdos *pdo_data)
{
	union get_pdo_cmd
	{
		unsigned long long cmd_val;
		struct{
			unsigned int cmd	: 8;
			unsigned int len	: 8;
			unsigned int con	: 7;
			unsigned int ptnr	: 1;
			unsigned int offset	: 8;
			unsigned int num	: 2;
			unsigned int src_snk	: 1;
			unsigned int type	: 2;
		}s;
	}pdo_cmd;
	int ret=-1,i=0;
	unsigned char buf[64];
	unsigned ppdo = 0;

	if(fp_command > 0)
	{
		do
		{
			pdo_cmd.s.cmd = 0x10;
			pdo_cmd.s.len = 0;
			pdo_cmd.s.con = conn_num+1;
			pdo_cmd.s.ptnr = partner;
			pdo_cmd.s.offset = i;
			pdo_cmd.s.num = 0;
			pdo_cmd.s.src_snk = src_snk;
			pdo_cmd.s.type = type;
			
			snprintf(buf, sizeof(buf), "0x%llx", pdo_cmd.cmd_val);
			ret = write(fp_command,buf,sizeof(buf));
			if(ret)
			{
				ret = get_ucsi_response(buf);
				if(ret< 16)
					return -1;
				pdo_data->pdo[i] = buf[3] << 24 | buf[2] << 16 | buf[1] << 8 | buf[0];
				if((pdo_data->pdo[i] == 0) | (pdo_data->pdo[i] == ppdo))
					break;
				ppdo = pdo_data->pdo[i];
			}
			i++;
		}while(1);

	}
	
	*num_pdo = i;
	return i;
}
static int libtypec_dbgfs_get_cable_properties_ops(int conn_num, struct libtypec_cable_property *conn_cap)
{
	int ret=-1;
	unsigned char buf[64];
	if(fp_command > 0)
	{
		snprintf(buf, sizeof(buf), "0x%x", (conn_num + 1) << 16 | 0x11);
		ret = write(fp_command,buf,sizeof(buf));
		if(ret)
		{
			ret = get_ucsi_response(buf);
			if(ret < 16)
				ret = -1;
			// Copy the entire buf into cap_data
			memcpy(conn_cap, buf, ret);
		}
	}
	return ret;
}
static int libtypec_dbgs_get_connector_status_ops(int conn_num, struct libtypec_connector_status *conn_sts)
{
	int ret=-1;
	unsigned char buf[64];

	if(fp_command > 0)
	{
		snprintf(buf, sizeof(buf), "0x%x", (conn_num + 1) << 16 | 0x12);
		ret = write(fp_command,buf,sizeof(buf));
		if(ret)
		{
			ret = get_ucsi_response(buf);
			if(ret < 16)
				ret = -1;
			// Copy the entire buf into cap_data
			memcpy(conn_sts, buf, ret);
		}
	}
    return ret;
}
static int libtypec_dbgfs_set_uor_ops(unsigned char conn_num, unsigned char uor)
{
	int ret=-1;
	unsigned char buf[64];
	union set_uor_cmd
	{
		struct {
			unsigned int cmd : 8;
			unsigned int data_leng : 8;
			unsigned int con_num : 7;
			unsigned int uor_type : 3;
		};
		unsigned int uor_cmd;
	}setuorcmd;

	if(fp_command > 0)
	{
		setuorcmd.cmd = 0x09;
		setuorcmd.data_leng = 0x00;
		setuorcmd.con_num = conn_num + 1;
		setuorcmd.uor_type = 0x4 | uor;

		snprintf(buf, sizeof(buf), "0x%x", setuorcmd.uor_cmd);
		ret = write(fp_command,buf,sizeof(buf));
		if(ret)
		{
			ret = get_ucsi_response(buf);
			if(ret < 16)
				ret = -1;
		}
	}
	return ret;
}

static int libtypec_dbgfs_set_pdr_ops(unsigned char conn_num, unsigned char pdr)
{
	int ret=-1;
	unsigned char buf[64];
	union set_pdr_cmd
	{
		struct {
			unsigned int cmd : 8;
			unsigned int data_leng : 8;
			unsigned int con_num : 7;
			unsigned int pdr_type : 3;
		};
		unsigned int pdr_cmd;
	}setpdrcmd;

	if(fp_command > 0)
	{
		setpdrcmd.cmd = 0x0B;
		setpdrcmd.data_leng = 0x00;
		setpdrcmd.con_num = conn_num + 1;
		setpdrcmd.pdr_type = pdr;
		snprintf(buf, sizeof(buf), "0x%x", setpdrcmd.pdr_cmd);
		ret = write(fp_command,buf,sizeof(buf));
		if(ret)
		{
			ret = get_ucsi_response(buf);
			if(ret < 16)
				ret = -1;
		}
	}
    return ret;
}
static int libtypec_dbgfs_set_ccom_ops(unsigned char conn_num, unsigned char ccom)
{
	int ret=-1;
	unsigned char buf[64];
	union set_ccom_cmd
	{
		struct {
			unsigned int cmd : 8;
			unsigned int data_leng : 8;
			unsigned int con_num : 7;
			unsigned int ccom_type : 3;
		};
		unsigned int ccom_cmd;
	}setccomcmd;

	if(fp_command > 0)
	{
		setccomcmd.cmd = 0x08;
		setccomcmd.data_leng = 0x00;
		setccomcmd.con_num = conn_num + 1;
		setccomcmd.ccom_type = ccom ;
		snprintf(buf, sizeof(buf), "0x%x", setccomcmd.ccom_cmd);
		ret = write(fp_command,buf,sizeof(buf));
		if(ret)
		{
			ret = get_ucsi_response(buf);
			if(ret < 16)
				ret = -1;
		}
	}
    return ret;
}

static int libtypec_dbgfs_get_lpm_info_ops(unsigned char conn_num, struct libtypec_get_lpm_ppm_info *lpm_ppm_info)
{
	int ret=-1;
	unsigned char buf[64];

	if(fp_command > 0)
	{
		snprintf(buf, sizeof(buf), "0x%x", (conn_num + 1) << 16 | 0x22);
		ret = write(fp_command,buf,sizeof(buf));
		if(ret)
		{
			ret = get_ucsi_response(buf);
			if(ret < 16)
				ret = -1;
			// Copy the entire buf into cap_data
			memcpy(lpm_ppm_info, buf, ret);
		}
	}
    return ret;
}
static int libtypec_dbgfs_get_error_status_ops(unsigned char conn_num, struct libtypec_get_error_status *error_status)
{
	int ret=-1;
	unsigned char buf[64];

	if(fp_command > 0)
	{
		snprintf(buf, sizeof(buf), "0x%x", (conn_num + 1) << 16 | 0x13);
		ret = write(fp_command,buf,sizeof(buf));
		if(ret)
		{
			ret = get_ucsi_response(buf);
			if(ret < 16)
				ret = -1;
			// Copy the entire buf into cap_data
			memcpy(error_status, buf, ret);
		}
	}
    return ret;
}
static int libtypec_dbgfs_set_new_cam_ops(unsigned char conn_num, unsigned char entry_exit, unsigned char new_cam, unsigned int am_spec)
{
	int ret=-1;
	unsigned char buf[64];
	union set_new_cam_cmd
	{
		struct {
			unsigned int cmd : 8;
			unsigned int data_leng : 8;
			unsigned int con_num : 7;
			unsigned int entry_exit : 1;
			unsigned int new_cam : 8;
			unsigned int am_spec;
		};
		unsigned long int newcam_cmd;
	}setnewcamcmd;

	if(fp_command > 0)
	{
		setnewcamcmd.cmd = 0x0F;
		setnewcamcmd.data_leng = 0x00;
		setnewcamcmd.con_num = conn_num + 1;
		setnewcamcmd.entry_exit = entry_exit;
		setnewcamcmd.new_cam = new_cam;
		setnewcamcmd.am_spec = am_spec;
		snprintf(buf, sizeof(buf), "0x%lx", setnewcamcmd.newcam_cmd);
		printf("===> %s\n", buf);
		ret = write(fp_command,buf,sizeof(buf));
		if(ret)
		{
			ret = get_ucsi_response(buf);
			if(ret < 16)
				ret = -1;
		}
	}
    return ret;
}

static int libtypec_dbgfs_get_cam_cs_ops(unsigned char conn_num, unsigned char cam, struct libtypec_get_cam_cs *cam_cs)
{
	int ret=-1;
	unsigned char buf[64];
	union get_cam_cs_cmd
	{
		struct {
			unsigned int cmd : 8;
			unsigned int data_leng : 8;
			unsigned int conn_num : 7;
			unsigned int reserved : 1;
			unsigned int cam : 8;
		};
		unsigned int camcs_cmd;
	}getcamcscmd;


	if(fp_command > 0)
	{
		getcamcscmd.cmd = 0x18;
		getcamcscmd.data_leng  = 0x0;
		getcamcscmd.conn_num = conn_num;
		getcamcscmd.cam = cam;

		snprintf(buf, sizeof(buf), "0x%x", getcamcscmd.camcs_cmd);
		ret = write(fp_command,buf,sizeof(buf));
		if(ret)
		{
			ret = get_ucsi_response(buf);
			if(ret < 16)
				ret = -1;
			// Copy the entire buf into cap_data
			memcpy(cam_cs, buf, ret);
		}
	}
    return ret;
}

const struct libtypec_os_backend libtypec_lnx_dbgfs_backend = {
	.init = libtypec_dbgfs_init,
	.exit = libtypec_dbgfs_exit,
	.connector_reset = libtypec_dbgfs_connector_reset_ops,
	.get_capability_ops = libtypec_dbgfs_get_capability_ops,
	.get_conn_capability_ops = libtypec_dbgfs_get_conn_capability_ops,
	.get_alternate_modes = libtypec_dbgfs_get_alternate_modes,
	.get_cam_supported_ops = NULL,
	.get_current_cam_ops = libtypec_dbfs_get_current_cam_ops,
	.get_pdos_ops = libtypec_dbgfs_get_pdos_ops,
	.get_cable_properties_ops = libtypec_dbgfs_get_cable_properties_ops,
	.get_connector_status_ops = libtypec_dbgs_get_connector_status_ops,
	.get_pd_message_ops = NULL,
	.get_bb_status = NULL,
	.get_bb_data = NULL,
	.set_uor_ops = libtypec_dbgfs_set_uor_ops,
	.set_pdr_ops = libtypec_dbgfs_set_pdr_ops,
	.set_ccom_ops = libtypec_dbgfs_set_ccom_ops,
	.get_lpm_ppm_info_ops = libtypec_dbgfs_get_lpm_info_ops,
	.get_error_status_ops = libtypec_dbgfs_get_error_status_ops,
	.set_new_cam_ops = libtypec_dbgfs_set_new_cam_ops,
	.get_cam_cs_ops = libtypec_dbgfs_get_cam_cs_ops,
};
