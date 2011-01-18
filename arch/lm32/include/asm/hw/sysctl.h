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

#ifndef __HW_SYSCTL_H
#define __HW_SYSCTL_H

#define CSR_REG(x) ((void __iomem *)(x))

#define CSR_GPIO_IN		CSR_REG(0xe0001000)
#define CSR_GPIO_OUT		CSR_REG(0xe0001004)
#define CSR_GPIO_INTEN		CSR_REG(0xe0001008)

#define CSR_TIMER0_CONTROL	CSR_REG(0xe0001010)
#define CSR_TIMER0_COMPARE	CSR_REG(0xe0001014)
#define CSR_TIMER0_COUNTER	CSR_REG(0xe0001018)

#define CSR_TIMER1_CONTROL	CSR_REG(0xe0001020)
#define CSR_TIMER1_COMPARE	CSR_REG(0xe0001024)
#define CSR_TIMER1_COUNTER	CSR_REG(0xe0001028)

#define CSR_TIMER(id, reg) CSR_REG(0xe0001010 + (0x10 * (id)) + (reg))

#define CSR_TIMER_CONTROL(id) CSR_TIMER(id, 0x0)
#define CSR_TIMER_COMPARE(id) CSR_TIMER(id, 0x4)
#define CSR_TIMER_COUNTER(id) CSR_TIMER(id, 0x8)

#define TIMER_ENABLE		(0x01)
#define TIMER_AUTORESTART	(0x02)

#define CSR_ICAP		CSR_REG(0xe0001034)

#define ICAP_READY		(0x01)

#define ICAP_CE			(0x10000)
#define ICAP_WRITE		(0x20000)

#define CSR_CAPABILITIES	CSR_REG(0xe0001038)
#define CSR_SYSTEM_ID		CSR_REG(0xe000103c)

#endif /* __HW_SYSCTL_H */
