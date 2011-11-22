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

#ifndef __HW_INTERRUPTS_H
#define __HW_INTERRUPTS_H

#define IRQ_UARTRX		(0)
#define IRQ_GPIO		(1)
#define IRQ_TIMER0		(2)
#define IRQ_TIMER1		(3)
#define IRQ_AC97CRREQUEST	(4)
#define IRQ_AC97CRREPLY		(5)
#define IRQ_AC97DMAR		(6)
#define IRQ_AC97DMAW		(7)
#define IRQ_PFPU		(8)
#define IRQ_TMU			(9)
#define IRQ_ETHRX		(10)
#define IRQ_ETHTX		(11)
#define IRQ_VIDEOIN		(12)
#define IRQ_MIDIRX		(13)
#define IRQ_IR			(14)
#define IRQ_USB			(15)
#define	IRQ_PS2KEYBOARD		(16)
#define	IRQ_PS2MOUSE		(17)

#endif /* __HW_INTERRUPTS_H */
