#include <linux/reboot.h>
#include <linux/io.h>
#include <asm/hw/milkymist.h>

void machine_restart(char * __unused)
{
	/* Writing to CSR_SYSTEM_ID causes a system reset */
	iowrite32be(1, CSR_SYSTEM_ID);
	while(1)
		cpu_relax();
}
