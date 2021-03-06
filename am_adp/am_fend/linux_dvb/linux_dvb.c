#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/***************************************************************************
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * Description:
 */
/**\file
 * \brief Linux DVB前端设备
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-06-08: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_mem.h>
#include <am_misc.h>
#include "../am_fend_internal.h"
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
/*add for config define for linux dvb *.h*/
#include <am_config.h>
#include <linux/dvb/frontend.h>
#include <sys/ioctl.h>
#include <poll.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/


/****************************************************************************
 * Static functions
 ***************************************************************************/
static AM_ErrorCode_t dvb_open (AM_FEND_Device_t *dev, const AM_FEND_OpenPara_t *para);
static AM_ErrorCode_t dvb_set_mode (AM_FEND_Device_t *dev, int mode);
static AM_ErrorCode_t dvb_get_info (AM_FEND_Device_t *dev, struct dvb_frontend_info *info);
static AM_ErrorCode_t dvb_get_ts (AM_FEND_Device_t *dev, AM_DMX_Source_t *src);
static AM_ErrorCode_t dvb_set_para (AM_FEND_Device_t *dev, const struct dvb_frontend_parameters *para);
static AM_ErrorCode_t dvb_get_para (AM_FEND_Device_t *dev, struct dvb_frontend_parameters *para);
static AM_ErrorCode_t dvb_get_status (AM_FEND_Device_t *dev, fe_status_t *status);
static AM_ErrorCode_t dvb_get_snr (AM_FEND_Device_t *dev, int *snr);
static AM_ErrorCode_t dvb_get_ber (AM_FEND_Device_t *dev, int *ber);
static AM_ErrorCode_t dvb_get_strength (AM_FEND_Device_t *dev, int *strength);
static AM_ErrorCode_t dvb_wait_event (AM_FEND_Device_t *dev, struct dvb_frontend_event *evt, int timeout);
static AM_ErrorCode_t dvb_set_delay(AM_FEND_Device_t *dev,  int delay);
static AM_ErrorCode_t dvb_diseqc_reset_overload(AM_FEND_Device_t *dev);
static AM_ErrorCode_t dvb_diseqc_send_master_cmd(AM_FEND_Device_t *dev, struct dvb_diseqc_master_cmd* cmd);
static AM_ErrorCode_t dvb_diseqc_recv_slave_reply(AM_FEND_Device_t *dev, struct dvb_diseqc_slave_reply* reply);
static AM_ErrorCode_t dvb_diseqc_send_burst(AM_FEND_Device_t *dev, fe_sec_mini_cmd_t minicmd);
static AM_ErrorCode_t dvb_set_tone(AM_FEND_Device_t *dev, fe_sec_tone_mode_t tone);
static AM_ErrorCode_t dvb_set_voltage(AM_FEND_Device_t *dev, fe_sec_voltage_t voltage);
static AM_ErrorCode_t dvb_enable_high_lnb_voltage(AM_FEND_Device_t *dev, long arg);
static AM_ErrorCode_t dvb_close (AM_FEND_Device_t *dev);
static AM_ErrorCode_t dvb_set_prop (AM_FEND_Device_t *dev, const struct dtv_properties *prop);
static AM_ErrorCode_t dvb_get_prop (AM_FEND_Device_t *dev, struct dtv_properties *prop);
static AM_ErrorCode_t dvbsx_blindscan_scan(AM_FEND_Device_t *dev, struct dvbsx_blindscanpara *pbspara);
static AM_ErrorCode_t dvbsx_blindscan_getscanevent(AM_FEND_Device_t *dev, struct dvbsx_blindscanevent *pbsevent);
static AM_ErrorCode_t dvbsx_blindscan_cancel(AM_FEND_Device_t *dev);
static AM_ErrorCode_t dvb_fine_tune(AM_FEND_Device_t *dev, unsigned int freq);
static AM_ErrorCode_t dvb_set_cvbs_amp_out(AM_FEND_Device_t *dev, tuner_param_t *tuner_para);
static AM_ErrorCode_t dvb_get_atv_status(AM_FEND_Device_t *dev, atv_status_t *atv_status);
static AM_ErrorCode_t dvb_set_afc(AM_FEND_Device_t *dev, unsigned int afc);

/****************************************************************************
 * Static data
 ***************************************************************************/
const AM_FEND_Driver_t linux_dvb_fend_drv =
{
.open = dvb_open,
.set_mode = dvb_set_mode,
.get_info = dvb_get_info,
.get_ts   = dvb_get_ts,
.set_para = dvb_set_para,
.get_para = dvb_get_para,
.set_prop = dvb_set_prop,
.get_prop = dvb_get_prop,
.get_status = dvb_get_status,
.get_snr = dvb_get_snr,
.get_ber = dvb_get_ber,
.get_strength = dvb_get_strength,
.wait_event = dvb_wait_event,
.set_delay  = dvb_set_delay,
.diseqc_reset_overload = dvb_diseqc_reset_overload,
.diseqc_send_master_cmd = dvb_diseqc_send_master_cmd,
.diseqc_recv_slave_reply = dvb_diseqc_recv_slave_reply,
.diseqc_send_burst = dvb_diseqc_send_burst,
.set_tone = dvb_set_tone,
.set_voltage = dvb_set_voltage,
.enable_high_lnb_voltage = dvb_enable_high_lnb_voltage,	
.close = dvb_close,
.blindscan_scan = dvbsx_blindscan_scan,
.blindscan_getscanevent = dvbsx_blindscan_getscanevent,
.blindscan_cancel = dvbsx_blindscan_cancel,
.fine_tune = dvb_fine_tune,
.set_cvbs_amp_out = dvb_set_cvbs_amp_out,
.get_atv_status =dvb_get_atv_status,
.set_afc = dvb_set_afc
};

/****************************************************************************
 * Functions
 ***************************************************************************/
static AM_ErrorCode_t dvb_open (AM_FEND_Device_t *dev, const AM_FEND_OpenPara_t *para)
{
	char name[PATH_MAX];
	int fd, ret;

	snprintf(name, sizeof(name), "/dev/dvb0.frontend%d", dev->dev_no);
	
	fd = open(name, O_RDWR);
	if(fd==-1)
	{
		AM_DEBUG(1, "cannot open %s, error:%s", name, strerror(errno));
		return AM_FEND_ERR_CANNOT_OPEN;
	}
	
	dev->drv_data = (void*)(long)fd;

	if (para->mode != -1) {
		ret = dvb_set_mode(dev, para->mode);
		if (ret != AM_SUCCESS) {
			close(fd);
			return ret;
		}
	}

	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_set_mode (AM_FEND_Device_t *dev, int mode)
{
	int fd = (long)dev->drv_data;
	int ret;
	int fe_mode = SYS_UNDEFINED;
	if(mode < 0)
		return AM_SUCCESS;

/*
	SYS_UNDEFINED,
	SYS_DVBC_ANNEX_A,
	SYS_DVBC_ANNEX_B,
	SYS_DVBT,
	SYS_DSS,
	SYS_DVBS,
	SYS_DVBS2,
	SYS_DVBH,
	SYS_ISDBT,
	SYS_ISDBS,
	SYS_ISDBC,
	SYS_ATSC,
	SYS_ATSCMH,
	SYS_DTMB,
	SYS_CMMB,
	SYS_DAB,
	SYS_DVBT2,
	SYS_TURBO,
	SYS_DVBC_ANNEX_C,
	SYS_ANALOG
 */

	switch(mode)
	{
		case FE_QPSK:
			{
				/* process sec */
				fe_mode = SYS_DVBS;
				break;
			}
		case FE_QAM:
			fe_mode = SYS_DVBC_ANNEX_A;
			break;
		case FE_OFDM:
			fe_mode = SYS_DVBT;
			break;
		case FE_ATSC:
			fe_mode = SYS_ATSC;
			break;
		case FE_ANALOG:
			fe_mode = SYS_ANALOG;
			break;	
		case FE_DTMB:
			fe_mode = SYS_DTMB;
			break;
		case FE_ISDBT:
			fe_mode = SYS_ISDBT;
			break;
		default:
			break;
	}

	struct dtv_property p = {.cmd = DTV_DELIVERY_SYSTEM, .u.data = fe_mode};
	struct dtv_properties props = {.num = 1, .props = &p};

	printf("dvb_set_mode:%d\n", props.props[0].u.data);

	ret = dvb_set_prop(dev, &props);

	if(ret != 0){
		AM_DEBUG(1, "set mode %d failed (%d)\n", mode, errno);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}

	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_get_info (AM_FEND_Device_t *dev, struct dvb_frontend_info *info)
{
	int fd = (long)dev->drv_data;
	int ret;

	ret = ioctl(fd, FE_GET_INFO, info);
	if(ret != 0){
		return AM_FEND_ERR_NOT_SUPPORTED;
	}

	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_get_ts (AM_FEND_Device_t *dev, AM_DMX_Source_t *src)
{
	int fd = (long)dev->drv_data;
	int ret;

	struct dtv_property p = {.cmd = DTV_TS_INPUT, .u.data = 0};
	struct dtv_properties props = {.num = 1, .props = &p};

	printf("dvb_get_ts:%d\n", props.props[0].u.data);

	ret = dvb_get_prop(dev, &props);

	if(ret != 0){
		AM_DEBUG(1, "get ts failed (%d)\n", errno);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}
	*src = (AM_DMX_Source_t)p.u.data;
	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_set_para (AM_FEND_Device_t *dev, const struct dvb_frontend_parameters *para)
{
	int fd = (long)dev->drv_data;
	
	if(ioctl(fd, FE_SET_FRONTEND, para)==-1)
	{
		AM_DEBUG(1, "ioctl FE_SET_FRONTEND failed, error:%s", strerror(errno));
		return AM_FAILURE;
	}

	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_get_para (AM_FEND_Device_t *dev, struct dvb_frontend_parameters *para)
{
	int fd = (long)dev->drv_data;
	
	if(ioctl(fd, FE_GET_FRONTEND, para)==-1)
	{
		AM_DEBUG(1, "ioctl FE_GET_FRONTEND failed, error:%s", strerror(errno));
		return AM_FAILURE;
	}

	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_set_prop (AM_FEND_Device_t *dev, const struct dtv_properties *prop)
{
	int fd = (long)dev->drv_data;
	printf("set prop>>>>>>>>>>>>.\n");
	if(ioctl(fd, FE_SET_PROPERTY, prop)==-1)
	{
		AM_DEBUG(1, "ioctl FE_SET_PROPERTY failed, error:%s", strerror(errno));
		return AM_FAILURE;
	}

	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_get_prop (AM_FEND_Device_t *dev, struct dtv_properties *prop)
{
	int fd = (long)dev->drv_data;
	
	if(ioctl(fd, FE_GET_PROPERTY, prop)==-1)
	{
		AM_DEBUG(1, "ioctl FE_GET_PROPERTY failed, error:%s", strerror(errno));
		return AM_FAILURE;
	}

	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_get_status (AM_FEND_Device_t *dev, fe_status_t *status)
{
	int fd = (long)dev->drv_data;
	
	if(ioctl(fd, FE_READ_STATUS, status)==-1)
	{
		AM_DEBUG(1, "ioctl FE_READ_STATUS failed, error:%s", strerror(errno));
		return AM_FAILURE;
	}

	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_get_snr (AM_FEND_Device_t *dev, int *snr)
{
	int fd = (long)dev->drv_data;
	uint16_t v16;
	
	if(ioctl(fd, FE_READ_SNR, &v16)==-1)
	{
		AM_DEBUG(1, "ioctl FE_READ_SNR failed, error:%s", strerror(errno));
		return AM_FAILURE;
	}

	*snr = v16;
	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_get_ber (AM_FEND_Device_t *dev, int *ber)
{
	int fd = (long)dev->drv_data;
	uint32_t v32;
	
	if(ioctl(fd, FE_READ_BER, &v32)==-1)
	{
		AM_DEBUG(1, "ioctl FE_READ_BER failed, error:%s", strerror(errno));
		return AM_FAILURE;
	}

	*ber = v32;
	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_get_strength (AM_FEND_Device_t *dev, int *strength)
{
	int fd = (long)dev->drv_data;
	uint16_t v16;
	
	if(ioctl(fd, FE_READ_SIGNAL_STRENGTH, &v16)==-1)
	{
		AM_DEBUG(1, "ioctl FE_READ_SIGNAL_STRENGTH failed, error:%s", strerror(errno));
		return AM_FAILURE;
	}

	*strength = v16;
	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_wait_event (AM_FEND_Device_t *dev, struct dvb_frontend_event *evt, int timeout)
{
	int fd = (long)dev->drv_data;
	struct pollfd pfd;
	int ret;
	
	pfd.fd = fd;
	pfd.events = POLLIN;
	
	ret = poll(&pfd, 1, timeout);
	if(ret!=1)
	{
		return AM_FEND_ERR_TIMEOUT;
	}
	
	if(ioctl(fd, FE_GET_EVENT, evt)==-1)
	{
		AM_DEBUG(1, "ioctl FE_GET_EVENT failed, error:%s", strerror(errno));
		return AM_FAILURE;
	}
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_set_delay(AM_FEND_Device_t *dev,  int delay)
{
	int fd = (long)dev->drv_data;
	
#ifdef FE_SET_DELAY
	if(ioctl(fd, FE_SET_DELAY, delay)==-1)
	{
		AM_DEBUG(1, "ioctl FE_SET_DELAY failed, error:%s", strerror(errno));
		return AM_FAILURE;
	}
#endif
	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_diseqc_reset_overload(AM_FEND_Device_t *dev)
{
	int fd = (long)dev->drv_data;
	
	if(ioctl(fd, FE_DISEQC_RESET_OVERLOAD, 0)==-1)
	{
		AM_DEBUG(1, "ioctl FE_DISEQC_RESET_OVERLOAD failed, error:%s", strerror(errno));
		return AM_FAILURE;
	}

	return AM_SUCCESS;	
}

static AM_ErrorCode_t dvb_diseqc_send_master_cmd(AM_FEND_Device_t *dev, struct dvb_diseqc_master_cmd* cmd)
{
	int fd = (long)dev->drv_data;
	
	if(ioctl(fd, FE_DISEQC_SEND_MASTER_CMD, cmd)==-1)
	{
		AM_DEBUG(1, "ioctl FE_DISEQC_SEND_MASTER_CMD failed, error:%s", strerror(errno));
		return AM_FAILURE;
	}

	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_diseqc_recv_slave_reply(AM_FEND_Device_t *dev, struct dvb_diseqc_slave_reply* reply)
{
	int fd = (long)dev->drv_data;
	
	if(ioctl(fd, FE_DISEQC_RECV_SLAVE_REPLY, reply)==-1)
	{
		AM_DEBUG(1, "ioctl FE_DISEQC_RECV_SLAVE_REPLY failed, error:%s", strerror(errno));
		return AM_FAILURE;
	}

	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_diseqc_send_burst(AM_FEND_Device_t *dev, fe_sec_mini_cmd_t minicmd)
{
	int fd = (long)dev->drv_data;
	
	if(ioctl(fd, FE_DISEQC_SEND_BURST, minicmd)==-1)
	{
		AM_DEBUG(1, "ioctl FE_DISEQC_SEND_BURST failed, error:%s", strerror(errno));
		return AM_FAILURE;
	}

	return AM_SUCCESS;	
}

static AM_ErrorCode_t dvb_set_tone(AM_FEND_Device_t *dev, fe_sec_tone_mode_t tone)
{
	int fd = (long)dev->drv_data;
	
	if(ioctl(fd, FE_SET_TONE, tone)==-1)
	{
		AM_DEBUG(1, "ioctl FE_SET_TONE failed, error:%s", strerror(errno));
		return AM_FAILURE;
	}

	return AM_SUCCESS;	
}

static AM_ErrorCode_t dvb_set_voltage(AM_FEND_Device_t *dev, fe_sec_voltage_t voltage)
{
	int fd = (long)dev->drv_data;
	
	if(ioctl(fd, FE_SET_VOLTAGE, voltage)==-1)
	{
		AM_DEBUG(1, "ioctl FE_SET_VOLTAGE failed, error:%s", strerror(errno));
		return AM_FAILURE;
	}

	return AM_SUCCESS;	
}

static AM_ErrorCode_t dvb_enable_high_lnb_voltage(AM_FEND_Device_t *dev, long arg)
{
	int fd = (long)dev->drv_data;
	
	if(ioctl(fd, FE_ENABLE_HIGH_LNB_VOLTAGE, arg)==-1)
	{
		AM_DEBUG(1, "ioctl FE_ENABLE_HIGH_LNB_VOLTAGE failed, error:%s", strerror(errno));
		return AM_FAILURE;
	}

	return AM_SUCCESS;	
}

static AM_ErrorCode_t dvb_close (AM_FEND_Device_t *dev)
{
	int fd = (long)dev->drv_data;
	
	close(fd);
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t dvbsx_blindscan_scan(AM_FEND_Device_t *dev, struct dvbsx_blindscanpara *pbspara)
{
	int ret =AM_SUCCESS;
	#if 0
	int fd = (long)dev->drv_data;
	
	if(ioctl(fd, FE_SET_BLINDSCAN, pbspara)==-1)
	{
		AM_DEBUG(1, "ioctl dvbsx_blindscan_scan failed, error:%s", strerror(errno));
		return AM_FAILURE;
	}
	#else
	/*set propty*/
	struct dtv_properties prop;
	struct dtv_property *property = NULL;
	int num = 8;

	property = malloc(num * sizeof(struct dtv_property));

	prop.num = num;
	prop.props = property;
	/*set min fre*/
	(property+0)->cmd = DTV_BLIND_SCAN_MIN_FRE;
	(property+0)->u.data = pbspara->minfrequency;
	/*set max fre*/
	(property+1)->cmd = DTV_BLIND_SCAN_MAX_FRE;
	(property+1)->u.data = pbspara->maxfrequency;

	/*set min rate*/
	(property+2)->cmd = DTV_BLIND_SCAN_MIN_SRATE;
	(property+2)->u.data = pbspara->minSymbolRate;

	/*set max rate*/
	(property+3)->cmd = DTV_BLIND_SCAN_MAX_SRATE;
	(property+3)->u.data = pbspara->maxSymbolRate;
	/*set fre range*/
	(property+4)->cmd = DTV_BLIND_SCAN_FRE_RANGE;
	(property+4)->u.data = pbspara->frequencyRange;
	/*set fre step*/
	(property+5)->cmd = DTV_BLIND_SCAN_FRE_STEP;
	(property+5)->u.data = pbspara->frequencyStep;
	/*set time out*/
	(property+6)->cmd = DTV_BLIND_SCAN_TIMEOUT;
	(property+6)->u.data = pbspara->timeout;
	/*set start blind scan*/
	(property+7)->cmd = DTV_START_BLIND_SCAN;
	(property+7)->u.data = 0;
	for (num = 0; num < 8; num++) {
		AM_DEBUG(1, "set start blind num[%d] cmd[%d]data[%d]\r\n", num, (property+num)->cmd, (property+num)->u.data);
	}
	ret = dvb_set_prop(dev, &prop);
	if (ret != AM_SUCCESS) {
		AM_DEBUG(1, "set start blind cmd error\n");
	}
	if (property != NULL) {
		free(property);
		property = NULL;	
	}
	#endif
	return ret;	
}

static AM_ErrorCode_t dvbsx_blindscan_getscanevent(AM_FEND_Device_t *dev, struct dvbsx_blindscanevent *pbsevent)
{
	int ret = AM_SUCCESS;
	#if 0
	int fd = (long)dev->drv_data;	
	
	if(ioctl(fd, FE_GET_BLINDSCANEVENT, pbsevent)!=0)
	{
		AM_DEBUG(1, "ioctl FE_GET_BLINDSCANEVENT failed, error:%s", strerror(errno));
		return AM_FAILURE;
	}
	#else

	struct dvb_frontend_event event;
	ret = dvb_wait_event(dev, &event, 200);
	if (event.status&BLINDSCAN_UPDATESTARTFREQ) {
		pbsevent->status = BLINDSCAN_UPDATESTARTFREQ;
		pbsevent->u.m_uistartfreq_khz = event.parameters.frequency;
	} else if (event.status&BLINDSCAN_UPDATEPROCESS) {
		pbsevent->status = BLINDSCAN_UPDATEPROCESS;
		pbsevent->u.m_uiprogress = event.parameters.frequency;	
	} else if (BLINDSCAN_UPDATERESULTFREQ) {
		pbsevent->status = BLINDSCAN_UPDATERESULTFREQ;
	    memcpy(&(pbsevent->u.parameters),
				&(event.parameters), sizeof(struct dvb_frontend_parameters));	
	}
	#endif
	return ret;	
}

static AM_ErrorCode_t dvbsx_blindscan_cancel(AM_FEND_Device_t *dev)
{
	int ret = AM_SUCCESS;
	#if 0
	int fd = (long)dev->drv_data;
	
	if(ioctl(fd, FE_SET_BLINDSCANCANCEl)==-1)
	{
		AM_DEBUG(1, "ioctl FE_SET_BLINDSCANCANCEl failed, error:%s", strerror(errno));
		return AM_FAILURE;
	}
	#else

	struct dtv_properties prop;
	struct dtv_property property;

	prop.num = 1;
	prop.props = &property;
	/*set min fre*/
	memset(&property, 0, sizeof(property));
	property.cmd = DTV_CANCEL_BLIND_SCAN;
	property.u.data = 0;

	ret = dvb_set_prop(dev, &prop);
	if (ret != AM_SUCCESS) {
		AM_DEBUG(1, "set cancel blind scan error\n");
	}
	#endif
	return ret;	
}

static AM_ErrorCode_t dvb_fine_tune(AM_FEND_Device_t *dev, unsigned int freq)
{
	int fd = (long)dev->drv_data;

	if(ioctl(fd, FE_FINE_TUNE, &freq)==-1)
	{
		AM_DEBUG(1, "ioctl FE_FINE_TUNE failed, errno: %s", strerror(errno));
		return AM_FAILURE;
	}

	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_set_cvbs_amp_out(AM_FEND_Device_t *dev, tuner_param_t *tuner_para)
{
	int fd = (long)dev->drv_data;
	if(ioctl(fd, FE_SET_PARAM_BOX, tuner_para)==-1)
	{
		AM_DEBUG(1, "ioctl FE_FINE_TUNE failed, errno: %s", strerror(errno));
		return AM_FAILURE;
	}

	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_get_atv_status(AM_FEND_Device_t *dev, atv_status_t *atv_status)
{
	int fd = (int)dev->drv_data;
	if(ioctl(fd, FE_READ_ANALOG_STATUS, atv_status)==-1)
	{
		AM_DEBUG(1, "ioctl FE_READ_ANALOG_STATUS failed, errno: %s", strerror(errno));
		return AM_FAILURE;
	}

	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_set_afc(AM_FEND_Device_t *dev, unsigned int afc)
{
	int fd = (long)dev->drv_data;

#ifdef FE_SET_AFC
	if(ioctl(fd, FE_SET_AFC, &afc)==-1)
	{
		AM_DEBUG(1, "ioctl FE_SET_AFC failed, errno: %s", strerror(errno));
		return AM_FAILURE;
	}
#endif

	return AM_SUCCESS;
}



