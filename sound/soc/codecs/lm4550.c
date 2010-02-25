/*
 * ALSA SoC LM4550 codec support
 *
 * Copyright (C) 2010 Sebastien Bourdeauducq
 * Based on lm4550.c, (C) Analog Devices Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/ac97_codec.h>
#include <sound/initval.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#include "lm4550.h"

static unsigned int ac97_read(struct snd_soc_codec *codec,
	unsigned int reg);
static int ac97_write(struct snd_soc_codec *codec,
	unsigned int reg, unsigned int val);

static const char *lm4550_rec_sel[] = {"Mic", "CD", "Video", "AUX", "Line",
		"Stereo Mix", "Mono Mix", "Phone"};

static const struct soc_enum lm4550_cap_src =
	SOC_ENUM_DOUBLE(AC97_REC_SEL, 8, 0, 7, lm4550_rec_sel);

static const struct snd_kcontrol_new lm4550_snd_ac97_controls[] = {
SOC_DOUBLE("Master Playback Volume", AC97_MASTER, 8, 0, 31, 1),
SOC_SINGLE("Master Playback Switch", AC97_MASTER, 15, 1, 1),

SOC_DOUBLE("Headphone Playback Volume", AC97_HEADPHONE, 8, 0, 31, 1),
SOC_SINGLE("Headphone Playback Switch", AC97_HEADPHONE, 15, 1, 1),

SOC_DOUBLE("PCM Playback Volume", AC97_PCM, 8, 0, 31, 1),
SOC_SINGLE("PCM Playback Switch", AC97_PCM, 15, 1, 1),

SOC_DOUBLE("PCM Capture Volume", AC97_REC_GAIN, 8, 0, 31, 0),
SOC_SINGLE("PCM Capture Switch", AC97_REC_GAIN, 15, 1, 1),

SOC_SINGLE("Mono Playback Volume", AC97_MASTER_MONO, 0, 31, 1),
SOC_SINGLE("Mono Playback Switch", AC97_MASTER_MONO, 15, 1, 1),

SOC_SINGLE("Phone Capture Volume", AC97_PHONE, 0, 31, 1),
SOC_SINGLE("Phone Capture Switch", AC97_PHONE, 15, 1, 1),

SOC_SINGLE("Mic Volume", AC97_MIC, 0, 31, 1),
SOC_SINGLE("Mic Switch", AC97_MIC, 15, 1, 1),

SOC_DOUBLE("Center/LFE Playback Volume", AC97_CENTER_LFE_MASTER, 8, 0, 31, 1),
SOC_DOUBLE("Center/LFE Playback Switch", AC97_CENTER_LFE_MASTER, 15, 7, 1, 1),

SOC_ENUM("Capture Source", lm4550_cap_src),

SOC_SINGLE("Mic Boost Switch", AC97_MIC, 6, 1, 0),
};

static unsigned int ac97_read(struct snd_soc_codec *codec,
	unsigned int reg)
{
	return soc_ac97_ops.read(codec->ac97, reg);
}

static int ac97_write(struct snd_soc_codec *codec, unsigned int reg,
	unsigned int val)
{
	soc_ac97_ops.write(codec->ac97, reg, val);
	return 0;
}

struct snd_soc_dai lm4550_dai = {
	.name = "AC97",
	.ac97_control = 1,
	.playback = {
		.stream_name = "Playback",
		.channels_min = 2,
		.channels_max = 6,
		.rates = SNDRV_PCM_RATE_48000,
		.formats = SND_SOC_STD_AC97_FMTS, },
	.capture = {
		.stream_name = "Capture",
		.channels_min = 2,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_48000,
		.formats = SND_SOC_STD_AC97_FMTS, },
};
EXPORT_SYMBOL_GPL(lm4550_dai);

static int lm4550_soc_probe(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec;
	int ret = 0;

	printk(KERN_INFO "LM4550 SoC Audio Codec\n");

	socdev->card->codec = kzalloc(sizeof(struct snd_soc_codec), GFP_KERNEL);
	if (socdev->card->codec == NULL)
		return -ENOMEM;
	codec = socdev->card->codec;
	mutex_init(&codec->mutex);

	codec->name = "lm4550";
	codec->owner = THIS_MODULE;
	codec->dai = &lm4550_dai;
	codec->num_dai = 1;
	codec->write = ac97_write;
	codec->read = ac97_read;
	INIT_LIST_HEAD(&codec->dapm_widgets);
	INIT_LIST_HEAD(&codec->dapm_paths);

	ret = snd_soc_new_ac97_codec(codec, &soc_ac97_ops, 0);
	if (ret < 0) {
		printk(KERN_ERR "LM4550: failed to register AC97 codec\n");
		goto codec_err;
	}

	/* register pcms */
	ret = snd_soc_new_pcms(socdev, SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1);
	if (ret < 0)
		goto pcm_err;

	ac97_write(codec, AC97_RESET, 0xffff);

	/* Read out vendor ID to make sure it is LM4550 */
	if (ac97_read(codec, AC97_VENDOR_ID1) != 0x4e53)
		goto reset_err;

	if (ac97_read(codec, AC97_VENDOR_ID2) != 0x4350)
		goto reset_err;

	/* unmute captures and playbacks volume */
	ac97_write(codec, AC97_MASTER, 0x0000);
	ac97_write(codec, AC97_HEADPHONE, 0x0f0f);
	ac97_write(codec, AC97_PCM, 0x0000);
	ac97_write(codec, AC97_MIC, 0x0000);
	ac97_write(codec, AC97_REC_GAIN, 0x0f0f);

	snd_soc_add_controls(codec, lm4550_snd_ac97_controls,
				ARRAY_SIZE(lm4550_snd_ac97_controls));

	return 0;

reset_err:
	snd_soc_free_pcms(socdev);
pcm_err:
	snd_soc_free_ac97_codec(codec);
codec_err:
	kfree(socdev->card->codec);
	socdev->card->codec = NULL;
	return ret;
}

static int lm4550_soc_remove(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->card->codec;

	if (codec == NULL)
		return 0;

	snd_soc_dapm_free(socdev);
	snd_soc_free_pcms(socdev);
	snd_soc_free_ac97_codec(codec);
	kfree(codec);
	return 0;
}

struct snd_soc_codec_device soc_codec_dev_lm4550 = {
	.probe = 	lm4550_soc_probe,
	.remove = 	lm4550_soc_remove,
};
EXPORT_SYMBOL_GPL(soc_codec_dev_lm4550);

MODULE_DESCRIPTION("ASoC LM4550 driver");
MODULE_AUTHOR("Sebastien Bourdeauducq");
MODULE_LICENSE("GPL");
