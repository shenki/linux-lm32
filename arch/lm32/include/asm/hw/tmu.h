/*
 * Milkymist VJ SoC (Software)
 * Copyright (C) 2007, 2008, 2009, 2010 Sebastien Bourdeauducq
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __HW_TMU_H
#define __HW_TMU_H

#define CSR_TMU_CTL		(0xe0007000)
#define TMU_CTL_START		0x01
#define TMU_CTL_BUSY		0x01
#define TMU_CTL_CHROMAKEY	0x02

#define CSR_TMU_HMESHLAST	(0xe0007004)
#define CSR_TMU_VMESHLAST	(0xe0007008)
#define CSR_TMU_BRIGHTNESS	(0xe000700C)
#define CSR_TMU_CHROMAKEY	(0xe0007010)

#define TMU_BRIGHTNESS_MAX	(63)

#define CSR_TMU_VERTICESADR	(0xe0007014)
#define CSR_TMU_TEXFBUF		(0xe0007018)
#define CSR_TMU_TEXHRES		(0xe000701C)
#define CSR_TMU_TEXVRES		(0xe0007020)
#define CSR_TMU_TEXHMASK	(0xe0007024)
#define CSR_TMU_TEXVMASK	(0xe0007028)

#define TMU_MASK_NOFILTER	(0x3ffc0)
#define TMU_MASK_FULL		(0x3ffff)
#define TMU_FIXEDPOINT_SHIFT	(6)

#define CSR_TMU_DSTFBUF		(0xe000702C)
#define CSR_TMU_DSTHRES		(0xe0007030)
#define CSR_TMU_DSTVRES		(0xe0007034)
#define CSR_TMU_DSTHOFFSET	(0xe0007038)
#define CSR_TMU_DSTVOFFSET	(0xe000703C)
#define CSR_TMU_DSTSQUAREW	(0xe0007040)
#define CSR_TMU_DSTSQUAREH	(0xe0007044)

#define CSR_TMU_ALPHA		(0xe0007048)

#define TMU_ALPHA_MAX		(63)

struct tmu_vertex {
	int x;
	int y;
} __attribute__((packed));

#define TMU_MESH_MAXSIZE	128

/* Performance monitoring */

#define CSR_TMU_REQ_A		(0xe0007050)
#define CSR_TMU_HIT_A		(0xe0007054)
#define CSR_TMU_REQ_B		(0xe0007058)
#define CSR_TMU_HIT_B		(0xe000705C)
#define CSR_TMU_REQ_C		(0xe0007060)
#define CSR_TMU_HIT_C		(0xe0007064)
#define CSR_TMU_REQ_D		(0xe0007068)
#define CSR_TMU_HIT_D		(0xe000706C)

#endif /* __HW_TMU_H */

