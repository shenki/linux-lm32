/*
 * ASoC driver for Milkymist One and ML401
 *
 * Copyright (C) 2010 Sebastien Bourdeauducq
 * Based on bf5xx-ad1836, Copyright 2009 Analog Devices Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/pcm_params.h>

#include <asm/cacheflush.h>
#include <asm/irq.h>

#include "../codecs/lm4550.h"

#include "milkymist-pcm.h"
#include "milkymist-ac97.h"

static struct snd_soc_dai_link milkymist_mm1_dai = {
	.name = "LM4550",
	.stream_name = "LM4550",
	.cpu_dai = &milkymist_ac97_dai,
	.codec_dai = &lm4550_dai,
};

static struct snd_soc_card milkymist_mm1 = {
	.name = "Milkymist One/ML401 audio",
	.platform = &milkymist_ac97_soc_platform,
	.dai_link = &milkymist_mm1_dai,
	.num_links = 1,
};

static struct snd_soc_device milkymist_mm1_snd_devdata = {
	.card = &milkymist_mm1,
	.codec_dev = &soc_codec_dev_lm4550,
};

static struct platform_device *milkymist_mm1_snd_device;

static int __init milkymist_mm1_init(void)
{
	int ret;

	milkymist_mm1_snd_device = platform_device_alloc("soc-audio", -1);
	if (!milkymist_mm1_snd_device)
		return -ENOMEM;

	platform_set_drvdata(milkymist_mm1_snd_device, &milkymist_mm1_snd_devdata);
	milkymist_mm1_snd_devdata.dev = &milkymist_mm1_snd_device->dev;
	ret = platform_device_add(milkymist_mm1_snd_device);

	if (ret)
		platform_device_put(milkymist_mm1_snd_device);

	return ret;
}

static void __exit milkymist_mm1_exit(void)
{
	platform_device_unregister(milkymist_mm1_snd_device);
}

module_init(milkymist_mm1_init);
module_exit(milkymist_mm1_exit);

MODULE_AUTHOR("Sebastien Bourdeauducq");
MODULE_DESCRIPTION("ALSA SoC Milkymist One and ML401 board driver");
MODULE_LICENSE("GPL");
