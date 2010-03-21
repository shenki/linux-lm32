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

#ifndef __INTERRUPTS_H
#define __INTERRUPTS_H

#define IRQ_GPIO		(0)
#define IRQ_TIMER0		(1)
#define IRQ_TIMER1		(2)
#define IRQ_UARTRX		(3)
#define IRQ_UARTTX		(4)
#define IRQ_AC97CRREQUEST	(5)
#define IRQ_AC97CRREPLY		(6)
#define IRQ_AC97DMAR		(7)
#define IRQ_AC97DMAW		(8)
#define IRQ_PFPU		(9)
#define IRQ_TMU			(10)
#define IRQ_PS2KEYBOARD		(11)
#define IRQ_PS2MOUSE		(12)
#define IRQ_ETHRX		(13)
#define IRQ_ETHTX		(14)

#endif /* __INTERRUPTS_H */
