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

#ifndef _AD1980_H
#define _AD1980_H
/* Bit definition of Power-Down Control/Status Register */
#define ADC		0x0001
#define DAC		0x0002
#define ANL		0x0004
#define REF		0x0008
#define PR0		0x0100
#define PR1		0x0200
#define PR2		0x0400
#define PR3		0x0800
#define PR4		0x1000
#define PR5		0x2000
#define PR6		0x4000

extern struct snd_soc_dai lm4550_dai;
extern struct snd_soc_codec_device soc_codec_dev_lm4550;

#endif
