/*
 *  intelmid_vm_control.c - Intel Sound card driver for MID
 *
 *  Copyright (C) 2010 Intel Corp
 *  Authors:	Vinod Koul <vinod.koul@intel.com>
 *
 *  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * This file contains the control operations of msic vendors
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/pci.h>
#include <linux/file.h>
#include <linux/delay.h>
#include "intel_sst.h"
#include "intelmid_snd_control.h"

static int msic_init_card(void)
{
	struct sc_reg_access sc_access[] = {
		/* dmic configuration */
		{0x241, 0x85, 0},
		{0x242, 0x02, 0},
		/* audio paths config */
		{0x24C, 0x10, 0},
		{0x24D, 0x32, 0},
		/* PCM2 interface slots */
		/* preconfigured slots for 0-5 both tx, rx */
		{0x272, 0x10, 0},
		{0x273, 0x32, 0},
		{0x274, 0xFF, 0},
		{0x275, 0x10, 0},
		{0x276, 0x32, 0},
		{0x277, 0x54, 0},
		/*Sinc5 decimator*/
		{0x24E, 0x28, 0},
		/*TI vibra w/a settings*/
		{0x384, 0x80, 0},
		{0x385, 0x80, 0},
		/*vibra settings*/
		{0x267, 0x00, 0},
		{0x26A, 0x10, 0},
		{0x261, 0x00, 0},
		{0x264, 0x10, 0},
		/* pcm port setting */
		{0x278, 0x00, 0},
		{0x27B, 0x01, 0},
		{0x27C, 0x0a, 0},
		/* Set vol HSLRVOLCTRL, IHFVOL */
		{0x259, 0x08, 0},
		{0x25A, 0x08, 0},
		{0x25B, 0x08, 0},
		{0x25C, 0x08, 0},
		/* HSEPRXCTRL  Enable the headset left and right FIR filters  */
		{0x250, 0x30, 0},
		/* HSMIXER */
		{0x256, 0x11, 0},
		/* amic configuration */
		{0x249, 0x01, 0x0},
		{0x24A, 0x01, 0x0},
		/* unmask ocaudio/accdet interrupts */
		{0x1d, 0x00, 0x00},
		{0x1e, 0x00, 0x00},
	};
	snd_msic_ops.card_status = SND_CARD_INIT_DONE;
	sst_sc_reg_access(sc_access, PMIC_WRITE, 30);
	snd_msic_ops.pb_on = 0;
	snd_msic_ops.cap_on = 0;
	snd_msic_ops.input_dev_id = DMIC; /*def dev*/
	snd_msic_ops.output_dev_id = STEREO_HEADPHONE;
	pr_debug("msic init complete!!\n");
	return 0;
}

static int msic_power_up_pb(unsigned int device)
{
	struct sc_reg_access vaud[] = {
		/* turn on the audio power supplies */
		{0x0DB, 0x07, 0},
	};
	struct sc_reg_access pll[] = {
		/* turn on PLL */
		{0x240, 0x20, 0},
	};
	struct sc_reg_access vhs[] = {
		/*  VHSP */
		{0x0DC, 0xFF, 0},
		/*  VHSN */
		{0x0DD, 0x3F, 0},
	};
	struct sc_reg_access hsdac[] = {
		/*  disable driver */
		{0x25D, 0x0, 0x43},
		/* DAC CONFIG ; both HP, LP on */
		{0x257, 0x03, 0x03},
	};
	struct sc_reg_access hs_filter[] = {
		/* HSEPRXCTRL  Enable the headset left and right FIR filters  */
		{0x250, 0x30, 0},
		/* HSMIXER */
		{0x256, 0x11, 0},
	};
	struct sc_reg_access hs_enable[] = {
		/* enable driver */
		{0x25D, 0x3, 0x3},
		{0x26C, 0x0, 0x2},
		/* unmute the headset */
		{ 0x259, 0x80, 0x80},
		{ 0x25A, 0x80, 0x80},
	};
	struct sc_reg_access vihf[] = {
		/*  VIHF ON */
		{0x0C9, 0x27, 0x00},
	};
	struct sc_reg_access ihf_filter[] = {
		/*  disable driver */
		{0x25D, 0x00, 0x0C},
		/*Filer DAC enable*/
		{0x251, 0x03, 0x03},
		{0x257, 0x0C, 0x0C},
	};
	struct sc_reg_access ihf_en[] = {
		/*enable drv*/
		{0x25D, 0x0C, 0x0c},
	};
	struct sc_reg_access ihf_unmute[] = {
		/*unmute headset*/
		{0x25B, 0x80, 0x80},
		{0x25C, 0x80, 0x80},
	};
	struct sc_reg_access epdac[] = {
		/*  disable driver */
		{0x25D, 0x0, 0x43},
		/* DAC CONFIG ; both HP, LP on */
		{0x257, 0x03, 0x03},
	};
	struct sc_reg_access ep_enable[] = {
		/* enable driver */
		{0x25D, 0x40, 0x40},
		/* unmute the headset */
		{ 0x259, 0x80, 0x80},
		{ 0x25A, 0x80, 0x80},
	};
	struct sc_reg_access vib1_en[] = {
		/* enable driver, ADC */
		{0x25D, 0x10, 0x10},
		{0x264, 0x02, 0x02},
	};
	struct sc_reg_access vib2_en[] = {
		/* enable driver, ADC */
		{0x25D, 0x20, 0x20},
		{0x26A, 0x02, 0x02},
	};
	struct sc_reg_access pcm2_en[] = {
		/* enable pcm 2 */
		{0x27C, 0x1, 0x1},
	};
	int retval = 0;

	if (snd_msic_ops.card_status == SND_CARD_UN_INIT) {
		retval = msic_init_card();
		if (retval)
			return retval;
	}

	pr_debug("powering up pb.... Device %d\n", device);
	sst_sc_reg_access(vaud, PMIC_WRITE, 1);
	msleep(1);
	sst_sc_reg_access(pll, PMIC_WRITE, 1);
	msleep(1);
	switch (device) {
	case SND_SST_DEVICE_HEADSET:
		if (snd_msic_ops.output_dev_id == STEREO_HEADPHONE) {
			sst_sc_reg_access(vhs, PMIC_WRITE, 2);
			sst_sc_reg_access(hsdac, PMIC_READ_MODIFY, 2);
			sst_sc_reg_access(hs_filter, PMIC_WRITE, 2);
			sst_sc_reg_access(hs_enable, PMIC_READ_MODIFY, 4);
		} else {
			sst_sc_reg_access(epdac, PMIC_READ_MODIFY, 2);
			sst_sc_reg_access(hs_filter, PMIC_WRITE, 2);
			sst_sc_reg_access(ep_enable, PMIC_READ_MODIFY, 3);
		}
		snd_msic_ops.pb_on = 1;
		break;

	case SND_SST_DEVICE_IHF:
		sst_sc_reg_access(vihf, PMIC_WRITE, 1);
		sst_sc_reg_access(ihf_filter, PMIC_READ_MODIFY, 3);
		sst_sc_reg_access(ihf_en, PMIC_READ_MODIFY, 1);
		sst_sc_reg_access(ihf_unmute, PMIC_READ_MODIFY, 2);
		break;

	case SND_SST_DEVICE_VIBRA:
		sst_sc_reg_access(vib1_en, PMIC_READ_MODIFY, 2);
		break;

	case SND_SST_DEVICE_HAPTIC:
		sst_sc_reg_access(vib2_en, PMIC_READ_MODIFY, 2);
		break;

	default:
		pr_warn("Wrong Device %d, selected %d\n",
			       device, snd_msic_ops.output_dev_id);
	}
	return sst_sc_reg_access(pcm2_en, PMIC_READ_MODIFY, 1);
}

static int msic_power_up_cp(unsigned int device)
{
	struct sc_reg_access vaud[] = {
		/* turn on the audio power supplies */
		{0x0DB, 0x07, 0},
	};
	struct sc_reg_access pll[] = {
		/* turn on PLL */
		{0x240, 0x20, 0},
	};
	struct sc_reg_access dmic_bias[] = {
		/*  Turn on AMIC supply  */
		{0x247, 0xA0, 0xA0},
	};
	struct sc_reg_access dmic[] = {
		/* mic demux enable */
		{0x245, 0x3F, 0x3F},
		{0x246, 0x07, 0x07},

	};
	struct sc_reg_access amic_bias[] = {
		/*  Turn on AMIC supply  */
		{0x247, 0xFC, 0xFC},
	};
	struct sc_reg_access amic[] = {
		/*MIC EN*/
		{0x249, 0x01, 0x01},
		{0x24A, 0x01, 0x01},
		/*ADC EN*/
		{0x248, 0x05, 0x0F},

	};
	struct sc_reg_access pcm2[] = {
		/* enable pcm 2 */
		{0x27C, 0x1, 0x1},
	};
	struct sc_reg_access tx_on[] = {
		/*wait for mic to stabalize before turning on audio channels*/
		{0x24F, 0x3C, 0x0},
	};
	int retval = 0;

	if (snd_msic_ops.card_status == SND_CARD_UN_INIT) {
		retval = msic_init_card();
		if (retval)
			return retval;
	}

	pr_debug("powering up cp....%d\n", snd_msic_ops.input_dev_id);
	sst_sc_reg_access(vaud, PMIC_WRITE, 1);
	msleep(500);/*FIXME need optimzed value here*/
	sst_sc_reg_access(pll, PMIC_WRITE, 1);
	msleep(1);
	snd_msic_ops.cap_on = 1;
	if (snd_msic_ops.input_dev_id == AMIC) {
		sst_sc_reg_access(amic_bias, PMIC_READ_MODIFY, 1);
		msleep(1);
		sst_sc_reg_access(amic, PMIC_READ_MODIFY, 3);
	} else {
		sst_sc_reg_access(dmic_bias, PMIC_READ_MODIFY, 1);
		msleep(1);
		sst_sc_reg_access(dmic, PMIC_READ_MODIFY, 2);
	}
	msleep(1);
	sst_sc_reg_access(tx_on, PMIC_WRITE, 1);
	return sst_sc_reg_access(pcm2, PMIC_READ_MODIFY, 1);
}

static int msic_power_down(void)
{
	struct sc_reg_access power_dn[] = {
		/*  VHSP */
		{0x0DC, 0xC4, 0},
		/*  VHSN */
		{0x0DD, 0x04, 0},
		/*  VIHF */
		{0x0C9, 0x24, 0},
	};
	struct sc_reg_access pll[] = {
		/* turn off PLL*/
		{0x240, 0x00, 0x0},
	};
	struct sc_reg_access vaud[] = {
		/* turn off VAUD*/
		{0x0DB, 0x04, 0},
	};

	pr_debug("powering dn msic\n");
	snd_msic_ops.pb_on = 0;
	snd_msic_ops.cap_on = 0;
	sst_sc_reg_access(power_dn, PMIC_WRITE, 3);
	msleep(1);
	sst_sc_reg_access(pll, PMIC_WRITE, 1);
	msleep(1);
	sst_sc_reg_access(vaud, PMIC_WRITE, 1);
	return 0;
}

static int msic_power_down_pb(unsigned int device)
{
	struct sc_reg_access drv_enable[] = {
		{0x25D, 0x00, 0x00},
	};
	struct sc_reg_access hs_mute[] = {
		{0x259, 0x80, 0x80},
		{0x25A, 0x80, 0x80},
		{0x26C, 0x02, 0x02},
	};
	struct sc_reg_access hs_off[] = {
		{0x257, 0x00, 0x03},
		{0x250, 0x00, 0x30},
	};
	struct sc_reg_access ihf_mute[] = {
		{0x25B, 0x80, 0x80},
		{0x25C, 0x80, 0x80},
	};
	struct sc_reg_access ihf_off[] = {
		{0x257, 0x00, 0x0C},
		{0x251, 0x00, 0x03},
	};
	struct sc_reg_access vib1_off[] = {
		{0x264, 0x00, 0x82},
	};
	struct sc_reg_access vib2_off[] = {
		{0x26A, 0x00, 0x82},
	};

	pr_debug("powering dn pb for device %d\n", device);
	switch (device) {
	case SND_SST_DEVICE_HEADSET:
		snd_msic_ops.pb_on = 0;
		sst_sc_reg_access(hs_mute, PMIC_READ_MODIFY, 3);
		drv_enable[0].mask = 0x43;
		sst_sc_reg_access(drv_enable, PMIC_READ_MODIFY, 1);
		sst_sc_reg_access(hs_off, PMIC_READ_MODIFY, 2);
		break;

	case SND_SST_DEVICE_IHF:
		sst_sc_reg_access(ihf_mute, PMIC_READ_MODIFY, 2);
		drv_enable[0].mask = 0x0C;
		sst_sc_reg_access(drv_enable, PMIC_READ_MODIFY, 1);
		sst_sc_reg_access(ihf_off, PMIC_READ_MODIFY, 2);
		break;

	case SND_SST_DEVICE_VIBRA:
		sst_sc_reg_access(vib1_off, PMIC_READ_MODIFY, 2);
		drv_enable[0].mask = 0x10;
		sst_sc_reg_access(drv_enable, PMIC_READ_MODIFY, 1);
		break;

	case SND_SST_DEVICE_HAPTIC:
		sst_sc_reg_access(vib2_off, PMIC_READ_MODIFY, 2);
		drv_enable[0].mask = 0x20;
		sst_sc_reg_access(drv_enable, PMIC_READ_MODIFY, 1);
		break;
	}
	return 0;
}

static int msic_power_down_cp(unsigned int device)
{
	struct sc_reg_access dmic[] = {
		{0x247, 0x00, 0xA0},
		{0x245, 0x00, 0x38},
		{0x246, 0x00, 0x07},
	};
	struct sc_reg_access amic[] = {
		{0x248, 0x00, 0x05},
		{0x249, 0x00, 0x01},
		{0x24A, 0x00, 0x01},
		{0x247, 0x00, 0xA3},
	};
	struct sc_reg_access tx_off[] = {
		{0x24F, 0x00, 0x3C},
	};

	pr_debug("powering dn cp....\n");
	snd_msic_ops.cap_on = 0;
	sst_sc_reg_access(tx_off, PMIC_READ_MODIFY, 1);
	if (snd_msic_ops.input_dev_id == DMIC)
		sst_sc_reg_access(dmic, PMIC_READ_MODIFY, 3);
	else
		sst_sc_reg_access(amic, PMIC_READ_MODIFY, 4);
	return 0;
}

static int msic_set_selected_output_dev(u8 value)
{
	int retval = 0;

	pr_debug("msic set selected output:%d\n", value);
	snd_msic_ops.output_dev_id = value;
	if (snd_msic_ops.pb_on)
		msic_power_up_pb(SND_SST_DEVICE_HEADSET);
	return retval;
}

static int msic_set_selected_input_dev(u8 value)
{

	struct sc_reg_access sc_access_dmic[] = {
		{0x24C, 0x10, 0x0},
	};
	struct sc_reg_access sc_access_amic[] = {
		{0x24C, 0x76, 0x0},

	};
	int retval = 0;

	pr_debug("msic_set_selected_input_dev:%d\n", value);
	snd_msic_ops.input_dev_id = value;
	switch (value) {
	case AMIC:
		pr_debug("Selecting AMIC1\n");
		retval = sst_sc_reg_access(sc_access_amic, PMIC_WRITE, 1);
		break;
	case DMIC:
		pr_debug("Selecting DMIC1\n");
		retval = sst_sc_reg_access(sc_access_dmic, PMIC_WRITE, 1);
		break;
	default:
		return -EINVAL;

	}
	if (snd_msic_ops.cap_on)
		retval = msic_power_up_cp(SND_SST_DEVICE_CAPTURE);
	return retval;
}

static int msic_set_pcm_voice_params(void)
{
	return 0;
}

static int msic_set_pcm_audio_params(int sfreq, int word_size, int num_channel)
{
	return 0;
}

static int msic_set_audio_port(int status)
{
	return 0;
}

static int msic_set_voice_port(int status)
{
	return 0;
}

static int msic_set_mute(int dev_id, u8 value)
{
	return 0;
}

static int msic_set_vol(int dev_id, int value)
{
	return 0;
}

static int msic_get_mute(int dev_id, u8 *value)
{
	return 0;
}

static int msic_get_vol(int dev_id, int *value)
{
	return 0;
}

struct snd_pmic_ops snd_msic_ops = {
	.set_input_dev	=	msic_set_selected_input_dev,
	.set_output_dev =	msic_set_selected_output_dev,
	.set_mute	=	msic_set_mute,
	.get_mute	=	msic_get_mute,
	.set_vol	=	msic_set_vol,
	.get_vol	=	msic_get_vol,
	.init_card	=	msic_init_card,
	.set_pcm_audio_params	= msic_set_pcm_audio_params,
	.set_pcm_voice_params	= msic_set_pcm_voice_params,
	.set_voice_port = msic_set_voice_port,
	.set_audio_port = msic_set_audio_port,
	.power_up_pmic_pb =	msic_power_up_pb,
	.power_up_pmic_cp =	msic_power_up_cp,
	.power_down_pmic_pb =	msic_power_down_pb,
	.power_down_pmic_cp =	msic_power_down_cp,
	.power_down_pmic =	msic_power_down,
};
