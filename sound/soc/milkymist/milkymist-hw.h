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

#ifndef _MILKYMIST_SNDHW_H
#define _MILKYMIST_SNDHW_H

#define CSR_AC97_CRCTL		(0x80004000)

#define AC97_CRCTL_RQEN		(0x01)
#define AC97_CRCTL_WRITE	(0x02)

#define CSR_AC97_CRADDR		(0x80004004)
#define CSR_AC97_CRDATAOUT	(0x80004008)
#define CSR_AC97_CRDATAIN	(0x8000400C)

#define CSR_AC97_DCTL		(0x80004010)
#define CSR_AC97_DADDRESS	(0x80004014)
#define CSR_AC97_DREMAINING	(0x80004018)

#define CSR_AC97_UCTL		(0x80004020)
#define CSR_AC97_UADDRESS	(0x80004024)
#define CSR_AC97_UREMAINING	(0x80004028)

#define AC97_SCTL_EN		(0x01)

#define AC97_MAX_DMASIZE	(0x3fffc)

#endif
