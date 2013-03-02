#include <linux/kernel.h>
#include <linux/clocksource.h>

void __init time_init(void)
{
	clocksource_of_init();
}
