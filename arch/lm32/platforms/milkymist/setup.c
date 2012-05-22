#include <linux/kernel.h>

extern void __init milkymist_setup_early_printk(void);

void __init plat_setup_arch(void)
{
#ifdef CONFIG_EARLY_PRINTK
    milkymist_setup_early_printk();
#endif
}
