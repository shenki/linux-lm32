/*
 * Dynamic DMA mapping support.
 *
 * We never have any address translations to worry about, so this
 * is just alloc/free.
 */

#include <linux/types.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <asm/cacheflush.h>

struct dma_map_ops *dma_ops;

#if 0
dma_addr_t dma_map_single(struct device *dev, void *addr, size_t size,
			  enum dma_data_direction dir)
{
	dma_addr_t handle = virt_to_phys(addr);
	flush_dcache_range(handle, size);
	return handle;
}
EXPORT_SYMBOL(dma_map_single);
#endif
