/* Copyright (c) 2013-2016, The Linux Foundation. All rights reserved.

* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 and
* only version 2 as published by the Free Software Foundation.

* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#include <linux/init.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <sound/apr_audio-v2.h>
#include <sound/core.h>
#include <sound/soc.h>
#include <sound/q6asm-v2.h>
#include <sound/pcm.h>
#include <sound/initval.h>
#include <sound/control.h>
#include <sound/tlv.h>
#include <asm/dma.h>

#include "msm-pcm-routing-v2.h"

#define LOOPBACK_VOL_MAX_STEPS 0x2000
#define LOOPBACK_SESSION_MAX 4

static DEFINE_MUTEX(loopback_session_lock);
static const DECLARE_TLV_DB_LINEAR(loopback_rx_vol_gain, 0,
				LOOPBACK_VOL_MAX_STEPS);

struct msm_pcm_loopback {
	struct snd_pcm_substream *playback_substream;
	struct snd_pcm_substream *capture_substream;

	int instance;

	struct mutex lock;

	uint32_t samp_rate;
	uint32_t channel_mode;

	int playback_start;
	int capture_start;
	int session_id;
	struct audio_client *audio_client;
	int volume;
};

struct fe_dai_session_map {
	char stream_name[32];
	struct msm_pcm_loopback *loopback_priv;
};

static struct fe_dai_session_map session_map[LOOPBACK_SESSION_MAX] = {
	{ {}, NULL},
	{ {}, NULL},
	{ {}, NULL},
	{ {}, NULL},
};

static void stop_pcm(struct msm_pcm_loopback *pcm);
static int msm_pcm_loopback_get_session(struct snd_soc_pcm_runtime *rtd,
					struct msm_pcm_loopback **pcm);

static void msm_pcm_route_event_handler(enum msm_pcm_routing_event event,
					void *priv_data)
{
	struct msm_pcm_loopback *pcm = priv_data;

	BUG_ON(!pcm);

	pr_debug("%s: event 0x%x\n", __func__, event);

	switch (event) {
	case MSM_PCM_RT_EVT_DEVSWITCH:
		q6asm_cmd(pcm->audio_client, CMD_PAUSE);
		q6asm_cmd(pcm->audio_client, CMD_FLUSH);
		q6asm_run(pcm->audio_client, 0, 0, 0);
	default:
		pr_err("%s: default event 0x%x\n", __func__, event);
		break;
	}
}

static void msm_pcm_loopback_event_handler(uint32_t opcode, uint32_t token,
					   uint32_t *payload, void *priv)
{
	pr_debug("%s:\n", __func__);
	switch (opcode) {
	case APR_BASIC_RSP_RESULT: {
		switch (payload[0]) {
			break;
		default:
			break;
		}
	}
		break;
	default:
		pr_err("%s: Not Supported Event opcode[0x%x]\n",
			__func__, opcode);
		break;
	}
}

static int msm_loopback_session_mute_put(struct snd_kcontrol *kcontrol,
					 struct snd_ctl_elem_value *ucontrol)
{
	int ret = 0, n = 0;
	int mute = ucontrol->value.integer.value[0];
	struct msm_pcm_loopback *pcm = NULL;

	if ((mute < 0) || (mute > 1)) {
		pr_err(" %s Invalid arguments", __func__);
		ret = -EINVAL;
		goto done;
	}

	pr_debug("%s: mute=%d\n", __func__, mute);

	for (n = 0; n < LOOPBACK_SESSION_MAX; n++) {
		if (!strcmp(session_map[n].stream_name, "MultiMedia6"))
			pcm = session_map[n].loopback_priv;
	}
	if (pcm && pcm->audio_client) {
		ret = q6asm_set_mute(pcm->audio_client, mute);
		if (ret < 0)
			pr_err("%s: Send mute command failed rc=%d\n",
				__func__, ret);
	}
done:
	return ret;
}

static struct snd_kcontrol_new msm_loopback_controls[] = {
	SOC_SINGLE_EXT("HFP TX Mute", SND_SOC_NOPM, 0, 1, 0,
			NULL, msm_loopback_session_mute_put),
};

static int msm_pcm_loopback_probe(struct snd_soc_platform *platform)
{
	snd_soc_add_platform_controls(platform, msm_loopback_controls,
				      ARRAY_SIZE(msm_loopback_controls));

	return 0;
}
static int pcm_loopback_set_volume(struct msm_pcm_loopback *prtd, int volume)
{
	int rc = -EINVAL;

	pr_debug("%s: Setting volume 0x%x\n", __func__, volume);

	if (prtd && prtd->audio_client) {
		rc = q6asm_set_volume(prtd->audio_client, volume);
		if (rc < 0) {
			pr_err("%s: Send Volume command failed rc = %d\n",
				__func__, rc);
			return rc;
		}
		prtd->volume = volume;
	}
	return rc;
}

static int msm_pcm_loopback_get_session(struct snd_soc_pcm_runtime *rtd,
					struct msm_pcm_loopback **pcm)
{
	int ret = 0;
	int n, index = -1;

	dev_dbg(rtd->platform->dev, "%s: stream %s\n", __func__,
		rtd->dai_link->stream_name);

	mutex_lock(&loopback_session_lock);
	for (n = 0; n < LOOPBACK_SESSION_MAX; n++) {
		if (!strcmp(rtd->dai_link->stream_name,
		    session_map[n].stream_name)) {
			*pcm = session_map[n].loopback_priv;
			goto exit;
		}
		/*
		 * Store the min index value for allocating a new session.
		 * Here, if session stream name is not found in the
		 * existing entries after the loop iteration, then this
		 * index will be used to allocate the new session.
		 * This index variable is expected to point to the topmost
		 * available free session.
		 */
		if (!(session_map[n].stream_name[0]) && (index < 0))
			index = n;
	}

	if (index < 0) {
		dev_err(rtd->platform->dev, "%s: Max Sessions allocated\n",
				 __func__);
		ret = -EAGAIN;
		goto exit;
	}

	session_map[index].loopback_priv = kzalloc(
		sizeof(struct msm_pcm_loopback), GFP_KERNEL);
	if (!session_map[index].loopback_priv) {
		ret = -ENOMEM;
		goto exit;
	}

	strlcpy(session_map[index].stream_name,
		rtd->dai_link->stream_name,
		sizeof(session_map[index].stream_name));
	dev_dbg(rtd->platform->dev, "%s: stream %s index %d\n",
		__func__, session_map[index].stream_name, index);

	mutex_init(&session_map[index].loopback_priv->lock);
	*pcm = session_map[index].loopback_priv;
exit:
	mutex_unlock(&loopback_session_lock);
	return ret;
}

static int msm_pcm_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = snd_pcm_substream_chip(substream);
	struct msm_pcm_loopback *pcm = NULL;
	int ret = 0;
	uint16_t bits_per_sample = 16;
	struct msm_pcm_routing_evt event;
	struct asm_session_mtmx_strtr_param_window_v2_t asm_mtmx_strtr_window;
	uint32_t param_id;

	ret =  msm_pcm_loopback_get_session(rtd, &pcm);
	if (ret)
		return ret;

	mutex_lock(&pcm->lock);

	pcm->volume = 0x2000;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		pcm->playback_substream = substream;
	else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		pcm->capture_substream = substream;

	pcm->instance++;
	dev_dbg(rtd->platform->dev, "%s: pcm out open: %d,%d\n", __func__,
			pcm->instance, substream->stream);
	if (pcm->instance == 2) {
		struct snd_soc_pcm_runtime *soc_pcm_rx =
				pcm->playback_substream->private_data;
		struct snd_soc_pcm_runtime *soc_pcm_tx =
				pcm->capture_substream->private_data;
		if (pcm->audio_client != NULL)
			stop_pcm(pcm);

		pcm->audio_client = q6asm_audio_client_alloc(
				(app_cb)msm_pcm_loopback_event_handler, pcm);
		if (!pcm->audio_client) {
			dev_err(rtd->platform->dev,
				"%s: Could not allocate memory\n", __func__);
			mutex_unlock(&pcm->lock);
			return -ENOMEM;
		}
		pcm->session_id = pcm->audio_client->session;
		pcm->audio_client->perf_mode = false;
		ret = q6asm_open_loopback_v2(pcm->audio_client,
					     bits_per_sample);
		if (ret < 0) {
			dev_err(rtd->platform->dev,
				"%s: pcm out open failed\n", __func__);
			q6asm_audio_client_free(pcm->audio_client);
			mutex_unlock(&pcm->lock);
			return -ENOMEM;
		}
		event.event_func = msm_pcm_route_event_handler;
		event.priv_data = (void *) pcm;
		msm_pcm_routing_reg_phy_stream(soc_pcm_tx->dai_link->be_id,
			pcm->audio_client->perf_mode,
			pcm->session_id, pcm->capture_substream->stream);
		msm_pcm_routing_reg_phy_stream_v2(soc_pcm_rx->dai_link->be_id,
			pcm->audio_client->perf_mode,
			pcm->session_id, pcm->playback_substream->stream,
			event);
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			pcm->playback_substream = substream;
			ret = pcm_loopback_set_volume(pcm, pcm->volume);
			if (ret < 0)
				dev_err(rtd->platform->dev,
					"Error %d setting volume", ret);
		}
		/* Set to largest negative value */
		asm_mtmx_strtr_window.window_lsw = 0x00000000;
		asm_mtmx_strtr_window.window_msw = 0x80000000;
		param_id = ASM_SESSION_MTMX_STRTR_PARAM_RENDER_WINDOW_START_V2;
		q6asm_send_mtmx_strtr_window(pcm->audio_client,
					     &asm_mtmx_strtr_window,
					     param_id);
		/* Set to largest positive value */
		asm_mtmx_strtr_window.window_lsw = 0xffffffff;
		asm_mtmx_strtr_window.window_msw = 0x7fffffff;
		param_id = ASM_SESSION_MTMX_STRTR_PARAM_RENDER_WINDOW_END_V2;
		q6asm_send_mtmx_strtr_window(pcm->audio_client,
					     &asm_mtmx_strtr_window,
					     param_id);
	}
	dev_info(rtd->platform->dev, "%s: Instance = %d, Stream ID = %s\n",
			__func__ , pcm->instance, substream->pcm->id);
	runtime->private_data = pcm;

	mutex_unlock(&pcm->lock);

	return 0;
}

static void stop_pcm(struct msm_pcm_loopback *pcm)
{
	struct snd_soc_pcm_runtime *soc_pcm_rx;
	struct snd_soc_pcm_runtime *soc_pcm_tx;

	if (pcm->audio_client == NULL)
		return;
	q6asm_cmd(pcm->audio_client, CMD_CLOSE);

	if (pcm->playback_substream != NULL) {
		soc_pcm_rx = pcm->playback_substream->private_data;
		msm_pcm_routing_dereg_phy_stream(soc_pcm_rx->dai_link->be_id,
				SNDRV_PCM_STREAM_PLAYBACK);
	}
	if (pcm->capture_substream != NULL) {
		soc_pcm_tx = pcm->capture_substream->private_data;
		msm_pcm_routing_dereg_phy_stream(soc_pcm_tx->dai_link->be_id,
				SNDRV_PCM_STREAM_CAPTURE);
	}
	q6asm_audio_client_free(pcm->audio_client);
	pcm->audio_client = NULL;
}

static int msm_pcm_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct msm_pcm_loopback *pcm = runtime->private_data;
	struct snd_soc_pcm_runtime *rtd = snd_pcm_substream_chip(substream);
	int ret = 0, n;
	bool found = false;

	mutex_lock(&pcm->lock);

	dev_dbg(rtd->platform->dev, "%s: end pcm call:%d\n",
		__func__, substream->stream);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		pcm->playback_start = 0;
	else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		pcm->capture_start = 0;

	pcm->instance--;
	if (!pcm->playback_start || !pcm->capture_start) {
		dev_dbg(rtd->platform->dev, "%s: end pcm call\n", __func__);
		stop_pcm(pcm);
	}

	if (!pcm->instance) {
		mutex_lock(&loopback_session_lock);
		for (n = 0; n < LOOPBACK_SESSION_MAX; n++) {
			if (!strcmp(rtd->dai_link->stream_name,
					session_map[n].stream_name)) {
				found = true;
				break;
			}
		}
		if (found) {
			memset(session_map[n].stream_name, 0,
				sizeof(session_map[n].stream_name));
			mutex_unlock(&pcm->lock);
			mutex_destroy(&session_map[n].loopback_priv->lock);
			session_map[n].loopback_priv = NULL;
			kfree(pcm);
			dev_dbg(rtd->platform->dev, "%s: stream freed %s\n",
				__func__, rtd->dai_link->stream_name);
			mutex_unlock(&loopback_session_lock);
			return 0;
		}
		mutex_unlock(&loopback_session_lock);
	}
	mutex_unlock(&pcm->lock);
	return ret;
}

static int msm_pcm_prepare(struct snd_pcm_substream *substream)
{
	int ret = 0;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct msm_pcm_loopback *pcm = runtime->private_data;
	struct snd_soc_pcm_runtime *rtd = snd_pcm_substream_chip(substream);

	mutex_lock(&pcm->lock);

	dev_dbg(rtd->platform->dev, "%s: ASM loopback stream:%d\n",
		__func__, substream->stream);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (!pcm->playback_start)
			pcm->playback_start = 1;
	} else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		if (!pcm->capture_start)
			pcm->capture_start = 1;
	}
	mutex_unlock(&pcm->lock);

	return ret;
}

static int msm_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct msm_pcm_loopback *pcm = runtime->private_data;
	struct snd_soc_pcm_runtime *rtd = snd_pcm_substream_chip(substream);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		dev_dbg(rtd->platform->dev,
			"%s: playback_start:%d,capture_start:%d\n", __func__,
			pcm->playback_start, pcm->capture_start);
		if (pcm->playback_start && pcm->capture_start)
			q6asm_run_nowait(pcm->audio_client, 0, 0, 0);
		break;
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
	case SNDRV_PCM_TRIGGER_STOP:
		dev_dbg(rtd->platform->dev,
			"%s:Pause/Stop - playback_start:%d,capture_start:%d\n",
			__func__, pcm->playback_start, pcm->capture_start);
		if (pcm->playback_start && pcm->capture_start)
			q6asm_cmd_nowait(pcm->audio_client, CMD_PAUSE);
		break;
	default:
		pr_err("%s: default cmd %d\n", __func__, cmd);
		break;
	}

	return 0;
}

static struct snd_pcm_ops msm_pcm_ops = {
	.open           = msm_pcm_open,
	.close          = msm_pcm_close,
	.prepare        = msm_pcm_prepare,
	.trigger        = msm_pcm_trigger,
};

static int msm_pcm_volume_ctl_put(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	int rc = 0;
	struct snd_pcm_volume *vol = kcontrol->private_data;
	struct snd_pcm_substream *substream = vol->pcm->streams[0].substream;
	struct msm_pcm_loopback *prtd = substream->runtime->private_data;
	int volume = ucontrol->value.integer.value[0];

	rc = pcm_loopback_set_volume(prtd, volume);
	return rc;
}

static int msm_pcm_add_controls(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_pcm *pcm = rtd->pcm->streams[0].pcm;
	struct snd_pcm_volume *volume_info;
	struct snd_kcontrol *kctl;
	int ret = 0;

	dev_dbg(rtd->dev, "%s, Volume cntrl add\n", __func__);
	ret = snd_pcm_add_volume_ctls(pcm, SNDRV_PCM_STREAM_PLAYBACK,
				      NULL, 1,
				      rtd->dai_link->be_id,
				      &volume_info);
	if (ret < 0)
		return ret;
	kctl = volume_info->kctl;
	kctl->put = msm_pcm_volume_ctl_put;
	kctl->tlv.p = loopback_rx_vol_gain;
	return 0;
}

static int msm_asoc_pcm_new(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_card *card = rtd->card->snd_card;
	int ret = 0;

	if (!card->dev->coherent_dma_mask)
		card->dev->coherent_dma_mask = DMA_BIT_MASK(32);

	ret = msm_pcm_add_controls(rtd);
	if (ret)
		dev_err(rtd->dev, "%s, kctl add failed\n", __func__);
	return ret;
}

static struct snd_soc_platform_driver msm_soc_platform = {
	.ops            = &msm_pcm_ops,
	.pcm_new        = msm_asoc_pcm_new,
	.probe          = msm_pcm_loopback_probe,
};

static int msm_pcm_probe(struct platform_device *pdev)
{

	dev_dbg(&pdev->dev, "%s: dev name %s\n",
		__func__, dev_name(&pdev->dev));

	return snd_soc_register_platform(&pdev->dev,
				   &msm_soc_platform);
}

static int msm_pcm_remove(struct platform_device *pdev)
{
	snd_soc_unregister_platform(&pdev->dev);
	return 0;
}

static const struct of_device_id msm_pcm_loopback_dt_match[] = {
	{.compatible = "qcom,msm-pcm-loopback"},
	{}
};

static struct platform_driver msm_pcm_driver = {
	.driver = {
		.name = "msm-pcm-loopback",
		.owner = THIS_MODULE,
		.of_match_table = msm_pcm_loopback_dt_match,
	},
	.probe = msm_pcm_probe,
	.remove = msm_pcm_remove,
};

static int __init msm_soc_platform_init(void)
{
	return platform_driver_register(&msm_pcm_driver);
}
module_init(msm_soc_platform_init);

static void __exit msm_soc_platform_exit(void)
{
	platform_driver_unregister(&msm_pcm_driver);
}
module_exit(msm_soc_platform_exit);

MODULE_DESCRIPTION("PCM loopback platform driver");
MODULE_LICENSE("GPL v2");
