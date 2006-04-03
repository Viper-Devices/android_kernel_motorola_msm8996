/* dvb-usb-ids.h is part of the DVB USB library.
 *
 * Copyright (C) 2004-5 Patrick Boettcher (patrick.boettcher@desy.de) see
 * dvb-usb-init.c for copyright information.
 *
 * a header file containing define's for the USB device supported by the
 * various drivers.
 */
#ifndef _DVB_USB_IDS_H_
#define _DVB_USB_IDS_H_

/* Vendor IDs */
#define USB_VID_ADSTECH						0x06e1
#define USB_VID_ANCHOR						0x0547
#define USB_VID_WIDEVIEW					0x14aa
#define USB_VID_AVERMEDIA					0x07ca
#define USB_VID_COMPRO						0x185b
#define USB_VID_COMPRO_UNK					0x145f
#define USB_VID_CYPRESS						0x04b4
#define USB_VID_DIBCOM						0x10b8
#define USB_VID_DVICO						0x0fe9
#define USB_VID_EMPIA						0xeb1a
#define USB_VID_GRANDTEC					0x5032
#define USB_VID_HANFTEK						0x15f4
#define USB_VID_HAUPPAUGE					0x2040
#define USB_VID_HYPER_PALTEK				0x1025
#define USB_VID_KWORLD						0xeb2a
#define USB_VID_KYE							0x0458
#define USB_VID_MEDION						0x1660
#define USB_VID_PINNACLE					0x2304
#define USB_VID_VISIONPLUS					0x13d3
#define USB_VID_TWINHAN						0x1822
#define USB_VID_ULTIMA_ELECTRONIC			0x05d8

/* Product IDs */
#define USB_PID_ADSTECH_USB2_COLD			0xa333
#define USB_PID_ADSTECH_USB2_WARM			0xa334
#define USB_PID_AVERMEDIA_DVBT_USB_COLD		0x0001
#define USB_PID_AVERMEDIA_DVBT_USB_WARM		0x0002
#define USB_PID_AVERMEDIA_DVBT_USB2_COLD	0xa800
#define USB_PID_AVERMEDIA_DVBT_USB2_WARM	0xa801
#define USB_PID_COMPRO_DVBU2000_COLD		0xd000
#define USB_PID_COMPRO_DVBU2000_WARM		0xd001
#define USB_PID_COMPRO_DVBU2000_UNK_COLD	0x010c
#define USB_PID_COMPRO_DVBU2000_UNK_WARM	0x010d
#define USB_PID_DIBCOM_HOOK_DEFAULT			0x0064
#define USB_PID_DIBCOM_HOOK_DEFAULT_REENUM	0x0065
#define USB_PID_DIBCOM_MOD3000_COLD			0x0bb8
#define USB_PID_DIBCOM_MOD3000_WARM			0x0bb9
#define USB_PID_DIBCOM_MOD3001_COLD			0x0bc6
#define USB_PID_DIBCOM_MOD3001_WARM			0x0bc7
#define USB_PID_DIBCOM_STK7700				0x1e14
#define USB_PID_DIBCOM_STK7700_REENUM		0x1e15
#define USB_PID_DIBCOM_ANCHOR_2135_COLD		0x2131
#define USB_PID_GRANDTEC_DVBT_USB_COLD		0x0fa0
#define USB_PID_GRANDTEC_DVBT_USB_WARM		0x0fa1
#define USB_PID_KWORLD_VSTREAM_COLD			0x17de
#define USB_PID_KWORLD_VSTREAM_WARM			0x17df
#define USB_PID_TWINHAN_VP7041_COLD			0x3201
#define USB_PID_TWINHAN_VP7041_WARM			0x3202
#define USB_PID_TWINHAN_VP7020_COLD			0x3203
#define USB_PID_TWINHAN_VP7020_WARM			0x3204
#define USB_PID_TWINHAN_VP7045_COLD			0x3205
#define USB_PID_TWINHAN_VP7045_WARM			0x3206
#define USB_PID_TWINHAN_VP7021_COLD			0x3207
#define USB_PID_TWINHAN_VP7021_WARM			0x3208
#define USB_PID_DNTV_TINYUSB2_COLD			0x3223
#define USB_PID_DNTV_TINYUSB2_WARM			0x3224
#define USB_PID_ULTIMA_TVBOX_COLD			0x8105
#define USB_PID_ULTIMA_TVBOX_WARM			0x8106
#define USB_PID_ULTIMA_TVBOX_AN2235_COLD	0x8107
#define USB_PID_ULTIMA_TVBOX_AN2235_WARM	0x8108
#define USB_PID_ULTIMA_TVBOX_ANCHOR_COLD	0x2235
#define USB_PID_ULTIMA_TVBOX_USB2_COLD		0x8109
#define USB_PID_ULTIMA_TVBOX_USB2_WARM		0x810a
#define USB_PID_ULTIMA_TVBOX_USB2_FX_COLD	0x8613
#define USB_PID_ULTIMA_TVBOX_USB2_FX_WARM	0x1002
#define USB_PID_UNK_HYPER_PALTEK_COLD		0x005e
#define USB_PID_UNK_HYPER_PALTEK_WARM		0x005f
#define USB_PID_HANFTEK_UMT_010_COLD		0x0001
#define USB_PID_HANFTEK_UMT_010_WARM		0x0015
#define USB_PID_DTT200U_COLD				0x0201
#define USB_PID_DTT200U_WARM				0x0301
#define USB_PID_WT220U_COLD					0x0222
#define USB_PID_WT220U_WARM					0x0221
#define USB_PID_WT220U_ZL0353_COLD			0x022a
#define USB_PID_WT220U_ZL0353_WARM			0x022b
#define USB_PID_WINTV_NOVA_T_USB2_COLD		0x9300
#define USB_PID_WINTV_NOVA_T_USB2_WARM		0x9301
#define USB_PID_NEBULA_DIGITV				0x0201
#define USB_PID_DVICO_BLUEBIRD_LGDT			0xd820
#define USB_PID_DVICO_BLUEBIRD_LG064F_COLD		0xd500
#define USB_PID_DVICO_BLUEBIRD_LG064F_WARM		0xd501
#define USB_PID_DVICO_BLUEBIRD_LGZ201_COLD		0xdb00
#define USB_PID_DVICO_BLUEBIRD_LGZ201_WARM		0xdb01
#define USB_PID_DVICO_BLUEBIRD_TH7579_COLD		0xdb10
#define USB_PID_DVICO_BLUEBIRD_TH7579_WARM		0xdb11
#define USB_PID_DVICO_BLUEBIRD_DEE1601_COLD		0xdb50
#define USB_PID_DVICO_BLUEBIRD_DEE1601_WARM		0xdb51
#define USB_PID_DIGITALNOW_BLUEBIRD_DEE1601_COLD	0xdb54
#define USB_PID_DIGITALNOW_BLUEBIRD_DEE1601_WARM	0xdb55
#define USB_PID_MEDION_MD95700				0x0932
#define USB_PID_KYE_DVB_T_COLD				0x701e
#define USB_PID_KYE_DVB_T_WARM				0x701f
#define USB_PID_PCTV_200E					0x020e
#define USB_PID_PCTV_400E					0x020f

#endif
