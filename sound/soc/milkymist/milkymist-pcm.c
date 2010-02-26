/*
 * ASoC driver for Milkymist chips
 *
 * Copyright (C) 2010 Sebastien Bourdeauducq
 * Based on bf5xx-tdm, Copyright 2009 Analog Devices Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include <asm/io.h>
#include <asm/irq.h>

#include "milkymist-pcm.h"
#include "milkymist-hw.h"

static unsigned int initial_dremaining;
static unsigned int initial_uremaining;
static unsigned int downstream_period;
static struct snd_pcm_substream *downstream;
static struct snd_pcm_substream *upstream;

static irqreturn_t downstream_irq(int irq, void *data)
{
	struct snd_pcm_runtime *runtime = downstream->runtime;
	int fragsize_bytes = frames_to_bytes(runtime, runtime->period_size);

	downstream_period++;
	if(downstream_period == runtime->periods) {
		downstream_period = 0;
		out_be32(CSR_AC97_DADDRESS, runtime->dma_addr);
	}

	out_be32(CSR_AC97_DREMAINING, fragsize_bytes);
	initial_dremaining = fragsize_bytes;
	
	snd_pcm_period_elapsed(downstream);
	
	return IRQ_HANDLED;
}

static irqreturn_t upstream_irq(int irq, void *data)
{
	snd_pcm_period_elapsed(upstream);
	return IRQ_HANDLED;
}

#define MAX_BUFFER (2*AC97_MAX_DMASIZE)

static const struct snd_pcm_hardware milkymist_pcm_hardware = {
	.info = (SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_BLOCK_TRANSFER |
		SNDRV_PCM_INFO_RESUME | SNDRV_PCM_INFO_BLOCK_TRANSFER),
	.formats =		SNDRV_PCM_FMTBIT_S16_BE,
	.rates =		SNDRV_PCM_RATE_48000,
	.rate_min =		48000,
	.rate_max =		48000,
	.channels_min =		2,
	.channels_max =		2,
	.buffer_bytes_max =	MAX_BUFFER,
	.period_bytes_min =	1024,
	.period_bytes_max =	AC97_MAX_DMASIZE,
	.periods_min =		1,
	.periods_max =		64,
};

static int milkymist_pcm_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *hw_params)
{
	return snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(hw_params));
}

static int milkymist_pcm_hw_free(struct snd_pcm_substream *substream)
{
	return snd_pcm_lib_free_pages(substream);
}

static int milkymist_pcm_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	int fragsize_bytes = frames_to_bytes(runtime, runtime->period_size);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		downstream_period = 0;
		downstream = substream;
		out_be32(CSR_AC97_DADDRESS, runtime->dma_addr);
		out_be32(CSR_AC97_DREMAINING, fragsize_bytes);
		initial_dremaining = fragsize_bytes;
	} else {
		upstream = substream;
		out_be32(CSR_AC97_UADDRESS, runtime->dma_addr);
		out_be32(CSR_AC97_UREMAINING, fragsize_bytes);
		initial_uremaining = fragsize_bytes;
	}

	return 0;
}

static int milkymist_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	int ret = 0;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			out_be32(CSR_AC97_DCTL, AC97_SCTL_EN);
		else
			out_be32(CSR_AC97_UCTL, AC97_SCTL_EN);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			out_be32(CSR_AC97_DCTL, 0);
		else
			out_be32(CSR_AC97_UCTL, 0);
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static snd_pcm_uframes_t milkymist_pcm_pointer(struct snd_pcm_substream *substream)
{
	unsigned int diff;
	snd_pcm_uframes_t frames;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		diff = initial_dremaining - in_be32(CSR_AC97_DREMAINING);
		frames = diff / 4;
	} else {
		diff = initial_uremaining - in_be32(CSR_AC97_UREMAINING);
		frames = diff / 4;
	}
	return frames;
}

static int milkymist_pcm_open(struct snd_pcm_substream *substream)
{
	snd_soc_set_runtime_hwparams(substream, &milkymist_pcm_hardware);

	return 0;
}

struct snd_pcm_ops milkymist_pcm_ac97_ops = {
	.open           = milkymist_pcm_open,
	.ioctl          = snd_pcm_lib_ioctl,
	.hw_params      = milkymist_pcm_hw_params,
	.hw_free        = milkymist_pcm_hw_free,
	.prepare        = milkymist_pcm_prepare,
	.trigger        = milkymist_pcm_trigger,
	.pointer        = milkymist_pcm_pointer
};

static int milkymist_pcm_ac97_new(struct snd_card *card, struct snd_soc_dai *dai,
	struct snd_pcm *pcm)
{
	int ret;
	
	ret = request_irq(IRQ_AC97DMAR, downstream_irq,
		IRQF_DISABLED, "milkymist_pcm down", pcm);
	if(ret) return ret;
	
	ret = request_irq(IRQ_AC97DMAW, upstream_irq,
		IRQF_DISABLED, "milkymist_pcm up", pcm);
	if(ret) {
		free_irq(IRQ_AC97DMAR, pcm);
		return ret;
	}
	lm32_irq_unmask(IRQ_AC97DMAR);
	lm32_irq_unmask(IRQ_AC97DMAW);
	
	ret = snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_DEV,
		card->dev, MAX_BUFFER, MAX_BUFFER);
	if(ret) {
		free_irq(IRQ_AC97DMAR, pcm);
		free_irq(IRQ_AC97DMAW, pcm);
		return ret;
	}

	return ret;
}

static void milkymist_pcm_ac97_free(struct snd_pcm *pcm)
{
	free_irq(IRQ_AC97DMAR, pcm);
	free_irq(IRQ_AC97DMAW, pcm);
	snd_pcm_lib_preallocate_free_for_all(pcm);
}

struct snd_soc_platform milkymist_ac97_soc_platform = {
	.name           = "milkymist-audio",
	.pcm_ops        = &milkymist_pcm_ac97_ops,
	.pcm_new        = milkymist_pcm_ac97_new,
	.pcm_free       = milkymist_pcm_ac97_free,
};
EXPORT_SYMBOL_GPL(milkymist_ac97_soc_platform);

static int __init milkymist_pcm_ac97_init(void)
{
	return snd_soc_register_platform(&milkymist_ac97_soc_platform);
}
module_init(milkymist_pcm_ac97_init);

static void __exit milkymist_pcm_ac97_exit(void)
{
	snd_soc_unregister_platform(&milkymist_ac97_soc_platform);
}
module_exit(milkymist_pcm_ac97_exit);

MODULE_AUTHOR("Sebastien Bourdeauducq");
MODULE_DESCRIPTION("Milkymist PCM DMA module");
MODULE_LICENSE("GPL");
