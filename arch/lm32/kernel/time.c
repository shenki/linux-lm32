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

#include <asm/hw/interrupts.h>
#include <asm/hw/sysctl.h>

#define TIMER_CLOCKEVENT 0
#define TIMER_CLOCKSOURCE 1

static uint32_t milkymist_ticks_per_jiffy;

static inline uint32_t milkymist_timer_get_counter(unsigned int timer)
{
	return ioread32be(CSR_TIMER_COUNTER(timer));
}

static inline void milkymist_timer_set_counter(unsigned int timer, uint32_t value)
{
	iowrite32be(value, CSR_TIMER_COUNTER(timer));
}

static inline uint32_t milkymist_timer_get_compare(unsigned int timer)
{
	return ioread32be(CSR_TIMER_COMPARE(timer));
}

static inline void milkymist_timer_set_compare(unsigned int timer, uint32_t value)
{
	iowrite32be(value, CSR_TIMER_COMPARE(timer));
}

static inline void milkymist_timer_disable(unsigned int timer)
{
	iowrite32be(0, CSR_TIMER_CONTROL(timer));
}

static inline void milkymist_timer_enable(unsigned int timer, bool periodic)
{
	uint32_t val = TIMER_ENABLE;
	if (periodic);
		val |= TIMER_AUTORESTART;
	iowrite32be(val, CSR_TIMER_CONTROL(timer));
}

cycles_t get_cycles(void)
{
	return milkymist_timer_get_counter(TIMER_CLOCKSOURCE);
}

static cycle_t milkymist_clocksource_read(struct clocksource *cs)
{
	return get_cycles();
}

static struct clocksource milkymist_clocksource = {
	.name = "milkymist-timer",
	.rating = 200,
	.read = milkymist_clocksource_read,
	.mask = CLOCKSOURCE_MASK(32),
	.flags = CLOCK_SOURCE_IS_CONTINUOUS,
};

static irqreturn_t milkymist_clockevent_irq(int irq, void *devid)
{
	struct clock_event_device *cd = devid;

	if (cd->mode != CLOCK_EVT_MODE_PERIODIC)
		milkymist_timer_disable(TIMER_CLOCKEVENT);

	cd->event_handler(cd);

	return IRQ_HANDLED;
}

static void milkymist_clockevent_set_mode(enum clock_event_mode mode,
	struct clock_event_device *cd)
{
	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		milkymist_timer_disable(TIMER_CLOCKEVENT);
		milkymist_timer_set_counter(TIMER_CLOCKEVENT, 0);
		milkymist_timer_set_compare(TIMER_CLOCKEVENT, milkymist_ticks_per_jiffy);
	case CLOCK_EVT_MODE_RESUME:
		milkymist_timer_enable(TIMER_CLOCKEVENT, true);
		break;
	case CLOCK_EVT_MODE_ONESHOT:
	case CLOCK_EVT_MODE_SHUTDOWN:
		milkymist_timer_disable(TIMER_CLOCKEVENT);
		break;
	default:
		break;
	}
}

static int milkymist_clockevent_set_next(unsigned long evt,
	struct clock_event_device *cd)
{
	milkymist_timer_set_counter(TIMER_CLOCKEVENT, 0);
	milkymist_timer_set_compare(TIMER_CLOCKEVENT, evt);
	milkymist_timer_enable(TIMER_CLOCKEVENT, false);

	return 0;
}

static struct clock_event_device milkymist_clockevent = {
	.name = "milkymist-timer",
	.features = CLOCK_EVT_FEAT_PERIODIC,
	.set_next_event = milkymist_clockevent_set_next,
	.set_mode = milkymist_clockevent_set_mode,
	.rating = 200,
	.irq = IRQ_TIMER0,
};

static struct irqaction timer_irqaction = {
	.handler	= milkymist_clockevent_irq,
	.flags		= IRQF_TIMER,
	.name		= "milkymist-timerirq",
	.dev_id		= &milkymist_clockevent,
};

void __init time_init(void)
{
	int ret;

	milkymist_ticks_per_jiffy = DIV_ROUND_CLOSEST(CONFIG_CPU_CLOCK, HZ);

	clockevents_calc_mult_shift(&milkymist_clockevent, CONFIG_CPU_CLOCK, 5);
	milkymist_clockevent.min_delta_ns = clockevent_delta2ns(100, &milkymist_clockevent);
	milkymist_clockevent.max_delta_ns = clockevent_delta2ns(0xffff, &milkymist_clockevent);
	milkymist_clockevent.cpumask = cpumask_of(0);

	milkymist_timer_disable(TIMER_CLOCKSOURCE);
	milkymist_timer_set_compare(TIMER_CLOCKSOURCE, 0xffffffff);
	milkymist_timer_set_counter(TIMER_CLOCKSOURCE, 0);
	milkymist_timer_enable(TIMER_CLOCKSOURCE, true);

	clockevents_register_device(&milkymist_clockevent);

	ret = clocksource_register_hz(&milkymist_clocksource, CONFIG_CPU_CLOCK);

	if (ret)
		printk(KERN_ERR "Failed to register clocksource: %d\n", ret);

	setup_irq(IRQ_TIMER0, &timer_irqaction);
}
