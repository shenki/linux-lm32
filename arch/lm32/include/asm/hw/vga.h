/*
 * Milkymist VJ SoC (Software)
 * Copyright (C) 2007, 2008, 2009 Sebastien Bourdeauducq
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

#ifndef __HW_VGA_H
#define __HW_VGA_H

#define CSR_VGA_RESET 		(0x80003000)

#define VGA_RESET		(0x01)

#define CSR_VGA_HRES 		(0x80003004)
#define CSR_VGA_HSYNC_START	(0x80003008)
#define CSR_VGA_HSYNC_END	(0x8000300C)
#define CSR_VGA_HSCAN 		(0x80003010)

#define CSR_VGA_VRES 		(0x80003014)
#define CSR_VGA_VSYNC_START	(0x80003018)
#define CSR_VGA_VSYNC_END	(0x8000301C)
#define CSR_VGA_VSCAN 		(0x80003020)

#define CSR_VGA_BASEADDRESS	(0x80003024)
#define CSR_VGA_BASEADDRESS_ACT	(0x80003028)

#define CSR_VGA_BURST_COUNT	(0x8000302C)

#endif /* __HW_VGA_H */
