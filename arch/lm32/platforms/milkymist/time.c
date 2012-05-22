/*
 *  Copyright (C) 2011, Lars-Peter Clausen <lars@metafoo.de>
 *  Clockevent and clocksource driver for the Milkymist sysctl timers
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <linux/interrupt.h>
#include <linux/kernel.h>

#include <linux/clockchips.h>
#include <linux/clocksource.h>
#include <linux/timex.h>
#include <linux/io.h>

// TODO: use the DTS?
#define TIMER0_BASE		0xe0001800

#define TIMER0_CSR(x)		((void *)(TIMER0_BASE+(x)))

#define CSR_TIMER0_EN		TIMER0_CSR(0x00)

#define CSR_TIMER0_COUNT3	TIMER0_CSR(0x04)
#define CSR_TIMER0_COUNT2	TIMER0_CSR(0x08)
#define CSR_TIMER0_COUNT1	TIMER0_CSR(0x0C)
#define CSR_TIMER0_COUNT0	TIMER0_CSR(0x10)

#define CSR_TIMER0_RELOAD3	TIMER0_CSR(0x14)
#define CSR_TIMER0_RELOAD2	TIMER0_CSR(0x18)
#define CSR_TIMER0_RELOAD1	TIMER0_CSR(0x1C)
#define CSR_TIMER0_RELOAD0	TIMER0_CSR(0x20)

#define CSR_TIMER0_EV_STAT	TIMER0_CSR(0x24)
#define CSR_TIMER0_EV_PENDING	TIMER0_CSR(0x28)
#define CSR_TIMER0_EV_ENABLE	TIMER0_CSR(0x2C)

#define TIMER0_INTERRUPT	1


static uint32_t milkymist_ticks_per_jiffy;

static inline void milkymist_timer_set_counter(uint32_t value)
{
	iowrite32be((value & 0xff000000) >> 24, CSR_TIMER0_COUNT3);
	iowrite32be((value & 0x00ff0000) >> 16, CSR_TIMER0_COUNT2);
	iowrite32be((value & 0x0000ff00) >> 8, CSR_TIMER0_COUNT1);
	iowrite32be(value & 0x000000ff, CSR_TIMER0_COUNT0);
}

static inline void milkymist_timer_set_reload(uint32_t value)
{
	iowrite32be((value & 0xff000000) >> 24, CSR_TIMER0_RELOAD3);
	iowrite32be((value & 0x00ff0000) >> 16, CSR_TIMER0_RELOAD2);
	iowrite32be((value & 0x0000ff00) >> 8, CSR_TIMER0_RELOAD1);
	iowrite32be(value & 0x000000ff, CSR_TIMER0_RELOAD0);
}

static inline void milkymist_timer_disable(void)
{
	iowrite32be(0, CSR_TIMER0_EN);
}

static inline void milkymist_timer_enable(void)
{
	iowrite32be(1, CSR_TIMER0_EN);
}

cycles_t get_cycles(void)
{
	return 0;
}

static irqreturn_t milkymist_clockevent_irq(int irq, void *devid)
{
	struct clock_event_device *cd = devid;

	if (cd->mode != CLOCK_EVT_MODE_PERIODIC)
		milkymist_timer_disable();

	cd->event_handler(cd);
	
	iowrite32be(1, CSR_TIMER0_EV_PENDING);
	return IRQ_HANDLED;
}

static void milkymist_clockevent_set_mode(enum clock_event_mode mode,
	struct clock_event_device *cd)
{
	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		milkymist_timer_disable();
		milkymist_timer_set_counter(milkymist_ticks_per_jiffy);
		milkymist_timer_set_reload(milkymist_ticks_per_jiffy);
		milkymist_timer_enable();
		break;
	case CLOCK_EVT_MODE_RESUME:
		milkymist_timer_enable();
		break;
	case CLOCK_EVT_MODE_ONESHOT:
		milkymist_timer_disable();
		milkymist_timer_set_counter(milkymist_ticks_per_jiffy);
		milkymist_timer_set_reload(0);
		milkymist_timer_enable();
		break;
	case CLOCK_EVT_MODE_SHUTDOWN:
		milkymist_timer_disable();
		break;
	default:
		break;
	}
}

static int milkymist_clockevent_set_next(unsigned long evt,
	struct clock_event_device *cd)
{
	milkymist_timer_disable();
	milkymist_timer_set_counter(evt);
	milkymist_timer_enable();

	return 0;
}

static struct clock_event_device milkymist_clockevent = {
	.name = "milkymist-timer",
	.features = CLOCK_EVT_FEAT_PERIODIC,
	.set_next_event = milkymist_clockevent_set_next,
	.set_mode = milkymist_clockevent_set_mode,
	.rating = 200,
	.irq = TIMER0_INTERRUPT,
};

static struct irqaction timer_irqaction = {
	.handler	= milkymist_clockevent_irq,
	.flags		= IRQF_TIMER,
	.name		= "milkymist-timerirq",
	.dev_id		= &milkymist_clockevent,
};

void __init plat_time_init(void)
{
	milkymist_ticks_per_jiffy = DIV_ROUND_CLOSEST(CONFIG_CPU_CLOCK, HZ);

	clockevents_calc_mult_shift(&milkymist_clockevent, CONFIG_CPU_CLOCK, 5);
	milkymist_clockevent.min_delta_ns = clockevent_delta2ns(100, &milkymist_clockevent);
	milkymist_clockevent.max_delta_ns = clockevent_delta2ns(0xffff, &milkymist_clockevent);
	milkymist_clockevent.cpumask = cpumask_of(0);

	clockevents_register_device(&milkymist_clockevent);

	setup_irq(TIMER0_INTERRUPT, &timer_irqaction);
}
