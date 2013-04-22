/*
 * ASoC driver for phytec pcm051 development board (platform)
 *
 * Author:      Lars Poeschel <poeschel@lemonage.de>
 * Copyright:   (C) 2012 Lemonage Software GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#define DEBUG

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>

#include <asm/dma.h>
#include <asm/mach-types.h>

#include <asm/hardware/asp.h>
#include <mach/edma.h>
#include <mach/board-pcm051.h>

#include "davinci-pcm.h"
#include "davinci-i2s.h"
#include "davinci-mcasp.h"
#include "../codecs/wm8974.h"

#define AUDIO_FORMAT (SND_SOC_DAIFMT_I2S | \
		SND_SOC_DAIFMT_CBS_CFS | SND_SOC_DAIFMT_NB_NF)

#define PCM051_AUXCLK (25000000)
#define CODEC_CLOCK	12288000

static int pcm051_hw_params(struct snd_pcm_substream *substream,
			 struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	unsigned int pll_out = 0;
	int ret = 0;
	int rate, div;

	rate = params_rate(params);
	switch (params_rate(params)) {
	case 8000:
	case 16000:
		pll_out = 12288000;
		break;
	case 48000:
		pll_out = 22579200;
		break;
	case 96000:
		pll_out = 24576000;
		break;
	case 11025:
	case 22050:
	case 44100:
	case 88200:
		pll_out = 22579200;
		break;
	default:
		printk(KERN_ERR "audio clock is not matched pcm051 card\n");
	}

	/* set codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, AUDIO_FORMAT);
	if (ret < 0)
		return ret;

	/* set cpu DAI configuration */
	ret = snd_soc_dai_set_fmt(cpu_dai, AUDIO_FORMAT);
	if (ret < 0)
		return ret;

	/* set cpu CLKXDIV */
	div = PCM051_AUXCLK / (rate * 16 * 2);
	ret = snd_soc_dai_set_clkdiv(cpu_dai, DAVINCI_MCASP_CLKXDIV,
		PCM051_AUXCLK / (rate * 16 * 2));
	if (ret < 0)
		return ret;

	if (div/3 > 31) {
		ret = snd_soc_dai_set_pll(codec_dai, 0, 0, PCM051_AUXCLK/4,
		pll_out);
		if (ret < 0)
			return ret;
	} else if (div > 31) {
		ret = snd_soc_dai_set_pll(codec_dai, 0, 0, PCM051_AUXCLK/3,
		pll_out);
		if (ret < 0)
			return ret;
	} else {

	ret = snd_soc_dai_set_pll(codec_dai, 0, 0, PCM051_AUXCLK,
		pll_out);
		if (ret < 0)
			return ret;
	}
	return 0;
}

static struct snd_soc_ops pcm051_ops = {
	.hw_params = pcm051_hw_params,
};

/* phytec pcm051 machine dapm widgets */
static const struct snd_soc_dapm_widget wm8974_dapm_widgets[] = {
	SND_SOC_DAPM_SPK("Loudspeaker", NULL),
	SND_SOC_DAPM_LINE("Mono out", NULL),
	SND_SOC_DAPM_MIC("Mic Jack", NULL),
};

/* davinci-pcm051 machine audio_map connections to the codec pins */
static const struct snd_soc_dapm_route audio_map[] = {
	/* Loudspeaker connected to SPKOUTP, SPKOUTN */
	{"Loudspeaker", NULL, "SPKOUTP"},
	{"Loudspeaker", NULL, "SPKOUTN"},

	/* Mono out connected to MONOOUT */
	{"Mono out", NULL, "MONOOUT"},

	/* Mic connected to MICN */
	{"Mic Jack", NULL, "MICN"},
};

/* Logic for the wm8974 codec as connected on the phytec pcm051 */
static int pcm051_wm8974_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;

	/* Add davinci-pcm051 specific widgets */
	snd_soc_dapm_new_controls(dapm, wm8974_dapm_widgets,
				  ARRAY_SIZE(wm8974_dapm_widgets));

	/* Set up davinci-pcm051 specific audio path audio_map */
	snd_soc_dapm_add_routes(dapm, audio_map, ARRAY_SIZE(audio_map));

	/* not connected */
	snd_soc_dapm_disable_pin(dapm, "MICP");
	snd_soc_dapm_disable_pin(dapm, "AUX");

	/* always connected */
	snd_soc_dapm_enable_pin(dapm, "Loudspeaker");
	snd_soc_dapm_enable_pin(dapm, "Mono out");
	snd_soc_dapm_enable_pin(dapm, "Mic Jack");

	return 0;
}

static struct snd_soc_dai_link pcm051_dai_link = {
	.name = "WM8974",
	.stream_name = "WM8974",
	.cpu_dai_name = "davinci-mcasp.0",
	.codec_dai_name = "wm8974-hifi",
	.codec_name = "wm8974-codec.1-001a",
	.platform_name = "davinci-pcm-audio",
	.init = pcm051_wm8974_init,
	.ops = &pcm051_ops,
};

/* davinci pcm051 audio machine driver */
static struct snd_soc_card pcm051_snd_soc_card = {
	.name = "PCM051 audio",
	.dai_link = &pcm051_dai_link,
	.num_links = 1,
};

static struct platform_device *pcm051_snd_device;

static int __init pcm051_init(void)
{
	struct snd_soc_card *pcm051_snd_dev_data;
	int index;
	int ret;

	pcm051_snd_dev_data = &pcm051_snd_soc_card;
	index = 0;

	pcm051_snd_device = platform_device_alloc("soc-audio", index);
	if (!pcm051_snd_device)
		return -ENOMEM;

	platform_set_drvdata(pcm051_snd_device, pcm051_snd_dev_data);
	ret = platform_device_add(pcm051_snd_device);
	if (ret)
		platform_device_put(pcm051_snd_device);

	return ret;
}

static void __exit pcm051_exit(void)
{
	platform_device_unregister(pcm051_snd_device);
}

module_init(pcm051_init);
module_exit(pcm051_exit);

MODULE_AUTHOR("Lars Poeschel");
MODULE_DESCRIPTION("Phytec PCM051 development board ASoC driver");
MODULE_LICENSE("GPL");
