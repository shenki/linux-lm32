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

#ifndef __HW_MINIMAC_H
#define __HW_MINIMAC_H

#define CSR_MINIMAC_BASE		(0xe0008000)

#define CSR_MINIMAC_SETUP		(0x00)

#define MINIMAC_SETUP_RXRST		(0x1)
#define MINIMAC_SETUP_TXRST		(0x2)

#define CSR_MINIMAC_MDIO		(0x04)

#define MINIMAC_MDIO_DO			(0x1)
#define MINIMAC_MDIO_DI			(0x2)
#define MINIMAC_MDIO_OE			(0x4)
#define MINIMAC_MDIO_CLK		(0x8)

#define CSR_MINIMAC_STATE0		(0x08)
#define CSR_MINIMAC_ADDR0		(0x0C)
#define CSR_MINIMAC_COUNT0		(0x10)

#define CSR_MINIMAC_STATE1		(0x14)
#define CSR_MINIMAC_ADDR1		(0x18)
#define CSR_MINIMAC_COUNT1		(0x1C)

#define CSR_MINIMAC_STATE2		(0x20)
#define CSR_MINIMAC_ADDR2		(0x24)
#define CSR_MINIMAC_COUNT2		(0x28)

#define CSR_MINIMAC_STATE3		(0x2C)
#define CSR_MINIMAC_ADDR3		(0x30)
#define CSR_MINIMAC_COUNT3		(0x34)

#define MINIMAC_STATE_EMPTY		(0x0)
#define MINIMAC_STATE_LOADED		(0x1)
#define MINIMAC_STATE_PENDING		(0x2)

#define CSR_MINIMAC_TXADR		(0x38)
#define CSR_MINIMAC_TXREMAINING		(0x3C)

#endif /* __HW_MINIMAC_H */
