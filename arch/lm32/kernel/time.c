#include <linux/kernel.h>
#include <asm/time.h>

void __init time_init(void)
{
	plat_time_init();
}
