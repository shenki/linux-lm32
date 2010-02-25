/*
 * ASoC driver for Milkymist chips
 *
 * Copyright (C) 2010 Sebastien Bourdeauducq
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/delay.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/ac97_codec.h>
#include <sound/initval.h>
#include <sound/soc.h>

#include <asm/irq.h>
#include <asm/portmux.h>
#include <linux/mutex.h>
#include <linux/gpio.h>

#include "milkymist-ac97.h"

static unsigned short milkymist_ac97_read(struct snd_ac97 *ac97,
	unsigned short reg)
{
	/* TODO */
	return 0;
}

void milkymist_ac97_write(struct snd_ac97 *ac97, unsigned short reg,
	unsigned short val)
{
	/* TODO */
}

static void milkymist_ac97_warm_reset(struct snd_ac97 *ac97)
{
	pr_info("%s: Not implemented\n", __func__);
}

static void milkymist_ac97_cold_reset(struct snd_ac97 *ac97)
{
	pr_info("%s: Not implemented\n", __func__);
}

struct snd_ac97_bus_ops soc_ac97_ops = {
	.read	= milkymist_ac97_read,
	.write	= milkymist_ac97_write,
	.warm_reset	= milkymist_ac97_warm_reset,
	.reset	= milkymist_ac97_cold_reset,
};
EXPORT_SYMBOL_GPL(soc_ac97_ops);

struct snd_soc_dai milkymist_ac97_dai = {
	.name = "milkymist-ac97",
	.id = 0,
	.ac97_control = 1,
	.playback = {
		.stream_name = "AC97 Playback",
		.channels_min = 2,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_48000,
		.formats = SNDRV_PCM_FMTBIT_S16_BE, },
	.capture = {
		.stream_name = "AC97 Capture",
		.channels_min = 2,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_48000,
		.formats = SNDRV_PCM_FMTBIT_S16_BE, },
};
EXPORT_SYMBOL_GPL(milkymist_ac97_dai);

static int __init milkymist_ac97_init(void)
{
	return snd_soc_register_dai(&milkymist_ac97_dai);
}
module_init(milkymist_ac97_init);

static void __exit milkymist_ac97_exit(void)
{
	snd_soc_unregister_dai(&milkymist_ac97_dai);
}
module_exit(milkymist_ac97_exit);

MODULE_AUTHOR("Sebastien Bourdeauducq");
MODULE_DESCRIPTION("AC97 driver for Milkymist");
MODULE_LICENSE("GPL");
