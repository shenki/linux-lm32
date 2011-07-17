/*
 * ALSA SoC LM4550 codec support
 *
 * Copyright (C) 2010 Sebastien Bourdeauducq
 * Based on ad1980.c, (C) Analog Devices Inc.
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
#include <sound/pcm.h>
#include <sound/ac97_codec.h>
#include <sound/initval.h>
#include <sound/soc.h>

static const uint16_t lm4550_default_regs[] = {
	0x0d50, 0x8000, 0x8000, 0x8000, 0x0000, 0x0000, 0x8008, 0x8008,
	0x8808, 0x8808, 0x8808, 0x8808, 0x8808, 0x0000, 0x8000, 0x0000,
	0x0000, 0x0101, 0x0000, 0x0000, 0x0201, 0x0000, 0xbb80, 0x0000,
	0x0000, 0xbb80,
};

static const char *lm4550_rec_sel[] = { "Mic", "CD", "Video", "AUX", "Line",
		"Stereo Mix", "Mono Mix", "Phone" };

static const struct soc_enum lm4550_cap_src =
	SOC_ENUM_DOUBLE(AC97_REC_SEL, 8, 0, 7, lm4550_rec_sel);

static const struct snd_kcontrol_new lm4550_snd_ac97_controls[] = {
SOC_DOUBLE("Master Playback Volume", AC97_MASTER, 8, 0, 31, 1),
SOC_SINGLE("Master Playback Switch", AC97_MASTER, 15, 1, 1),

SOC_DOUBLE("Headphone Playback Volume", AC97_HEADPHONE, 8, 0, 31, 1),
SOC_SINGLE("Headphone Playback Switch", AC97_HEADPHONE, 15, 1, 1),

SOC_DOUBLE("PCM Playback Volume", AC97_PCM, 8, 0, 31, 1),
SOC_SINGLE("PCM Playback Switch", AC97_PCM, 15, 1, 1),

SOC_DOUBLE("PCM Capture Volume", AC97_REC_GAIN, 8, 0, 15, 0),
SOC_SINGLE("PCM Capture Switch", AC97_REC_GAIN, 15, 1, 1),

SOC_SINGLE("Mono Playback Volume", AC97_MASTER_MONO, 0, 31, 1),
SOC_SINGLE("Mono Playback Switch", AC97_MASTER_MONO, 15, 1, 1),

SOC_SINGLE("Phone Capture Volume", AC97_PHONE, 0, 31, 1),
SOC_SINGLE("Phone Capture Switch", AC97_PHONE, 15, 1, 1),

SOC_SINGLE("Mic Volume", AC97_MIC, 0, 31, 1),
SOC_SINGLE("Mic Switch", AC97_MIC, 15, 1, 1),

SOC_ENUM("Capture Source", lm4550_cap_src),

SOC_SINGLE("Mic Boost Switch", AC97_MIC, 6, 1, 0),
};

static bool lm4550_volatile_register(struct snd_soc_codec *codec,
	unsigned int reg)
{
	switch (reg) {
	case AC97_RESET:
	case AC97_VENDOR_ID1:
	case AC97_VENDOR_ID2:
		return true;
	default:
		break;
	}

	return false;
}

static unsigned int ac97_read(struct snd_soc_codec *codec,
	unsigned int reg)
{
	int ret;
	unsigned int val;

	if (reg >= codec->driver->reg_cache_size ||
		lm4550_volatile_register(codec, reg)) {
		return soc_ac97_ops.read(codec->ac97, reg);
	}

	ret = snd_soc_cache_read(codec, reg >> 1, &val);
	if (ret < 0)
		return -1;

	return val;
}

static int ac97_write(struct snd_soc_codec *codec, unsigned int reg,
	unsigned int val)
{
	int ret;

	if (reg < codec->driver->reg_cache_size) {
		ret = snd_soc_cache_write(codec, reg >> 1, val);
		if (ret < 0)
			return -1;
	}

	soc_ac97_ops.write(codec->ac97, reg, val);
	return 0;
}

static struct snd_soc_dai_driver lm4550_dai_driver = {
	.name = "AC97",
	.ac97_control = 1,
	.playback = {
		.stream_name = "Playback",
		.channels_min = 2,
		.channels_max = 6,
		.rates = SNDRV_PCM_RATE_48000,
		.formats = SND_SOC_STD_AC97_FMTS,
	},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 2,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_48000,
		.formats = SND_SOC_STD_AC97_FMTS,
	},
};

static int lm4550_codec_probe(struct snd_soc_codec *codec)
{
	int ret;
	uint16_t id1, id2;

	ret = snd_soc_new_ac97_codec(codec, &soc_ac97_ops, 0);
	if (ret < 0)
		return ret;

	ac97_write(codec, AC97_RESET, 0xffff);

	id1 = ac97_read(codec, AC97_VENDOR_ID1);
	id2 = ac97_read(codec, AC97_VENDOR_ID2);

	if (id1 != 0x4e53 || id2 != 0x4350) {
		ret = -ENODEV;
		goto reset_err;
	}

	return 0;

reset_err:
	snd_soc_free_ac97_codec(codec);
	return ret;
}

static int lm4550_codec_remove(struct snd_soc_codec *codec)
{
	snd_soc_free_ac97_codec(codec);
	return 0;
}

static struct snd_soc_codec_driver lm4550_codec_driver = {
	.probe = lm4550_codec_probe,
	.remove = lm4550_codec_remove,

	.reg_cache_size = ARRAY_SIZE(lm4550_default_regs),
	.reg_cache_default = lm4550_default_regs,
	.reg_word_size = sizeof(uint16_t),
	.reg_cache_step = 2,

	.read = ac97_read,
	.write = ac97_write,

	.controls = lm4550_snd_ac97_controls,
	.num_controls = ARRAY_SIZE(lm4550_snd_ac97_controls),
};

static int __devinit lm4550_probe(struct platform_device *pdev)
{
	return snd_soc_register_codec(&pdev->dev, &lm4550_codec_driver,
		&lm4550_dai_driver, 1);
}

static int __devexit lm4550_remove(struct platform_device *pdev)
{
	snd_soc_unregister_codec(&pdev->dev);
	return 0;
}

static struct platform_driver lm4550_driver = {
	.driver = {
		.name = "lm4550",
		.owner = THIS_MODULE,
	},

	.probe = lm4550_probe,
	.remove = __devexit_p(lm4550_remove),
};

static int __init lm4550_init(void)
{
	return platform_driver_register(&lm4550_driver);
}
module_init(lm4550_init);

static void __exit lm4550_exit(void)
{
	platform_driver_unregister(&lm4550_driver);
}
module_exit(lm4550_exit);

MODULE_DESCRIPTION("ASoC LM4550 driver");
MODULE_AUTHOR("Sebastien Bourdeauducq");
MODULE_LICENSE("GPL");
