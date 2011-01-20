/*
 * linux/drivers/net/milkymist_minimac.c
 */

#include <linux/etherdevice.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <asm/cacheflush.h>
#include <asm/hw/milkymist.h>

static int buffer_size = 0x4000;	/* 16KBytes */
module_param(buffer_size, int, 0);
MODULE_PARM_DESC(buffer_size, "DMA buffer allocation size");

#define	MAX_PACKET_RECEPTION_SLOT	4
#define	MAX_ETH_FRAME_SIZE		1536
#define	WATCHDOG_TIMEOUT		(5 * HZ)

const unsigned int crc_table[256] = {
	0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
	0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
	0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
	0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
	0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
	0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
	0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
	0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
	0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
	0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
	0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
	0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
	0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
	0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
	0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
	0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
	0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
	0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
	0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
	0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
	0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
	0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
	0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
	0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
	0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
	0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
	0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
	0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
	0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
	0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
	0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
	0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
	0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
	0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
	0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
	0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
	0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
	0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
	0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
	0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
	0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
	0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
	0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
	0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
	0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
	0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
	0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
	0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
	0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
	0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
	0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
	0x2d02ef8dL
};

#define DO1(buf) crc = crc_table[((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8);
#define DO2(buf)  DO1(buf); DO1(buf);
#define DO4(buf)  DO2(buf); DO2(buf);
#define DO8(buf)  DO4(buf); DO4(buf);

unsigned int crc32(const unsigned char *buffer, unsigned int len)
{
	unsigned int crc;
	crc = 0;
	crc = crc ^ 0xffffffffL;
	while(len >= 8) {
		DO8(buffer);
		len -= 8;
	}
	if(len) do {
		DO1(buffer);
	} while(--len);
	return crc ^ 0xffffffffL;
}

int get_frame(const unsigned char *src, int len )
{
	int i;
	unsigned int received_crc;
	unsigned int computed_crc;

	for(i=0;i<7;i++)
		if(*(src+i) != 0x55) return -1;

	if(*(src+7) != 0xd5) return -1;

	received_crc = ((unsigned int)src[len-1] << 24)
		|((unsigned int)src[len-2] << 16)
		|((unsigned int)src[len-3] <<  8)
		|((unsigned int)src[len-4]);

	computed_crc = crc32(src+8, len-12);

	if(received_crc != computed_crc) return -2;

	return (len-12);
}

struct _pkt_buf {
	int	state;	// State of the slot
	int	count;	// Reception byte count of the slot
	unsigned char buf[MAX_ETH_FRAME_SIZE+16];
};

/**
 * struct minimac - driver-tpate device structure
 * @iobase:	pointer to I/O memory region
 * @membase:	pointer to buffer memory region
 * @netdev:	pointer to network device structure
 * @napi:	NAPI structure
 * @rx_lock:	receive lock
 * @lock:	device lock
 */
struct minimac {
	void __iomem *iobase;
	union {
		void __iomem *membase;
		struct _pkt_buf *pkt_buf;
	};

	struct net_device *netdev;
	struct napi_struct napi;

	spinlock_t rx_lock;
	spinlock_t lock;

};


static int minimac_reset(struct minimac *dev)
{
	static unsigned char preamble[8] = {0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0xd5};
	int i;

	iowrite32be(MINIMAC_SETUP_RXRST | MINIMAC_SETUP_TXRST, CSR_MINIMAC_SETUP);

	for (i = 0; i < MAX_PACKET_RECEPTION_SLOT; ++i) {
		iowrite32be((unsigned int)dev->pkt_buf[i].buf, CSR_MINIMAC_ADDR0+i*12);
		iowrite32be(MINIMAC_STATE_LOADED, CSR_MINIMAC_STATE0+i*12);
	}

	memcpy_toio(dev->pkt_buf->buf, preamble, 8);		/* TX buff preamble */

	iowrite32be(0, CSR_MINIMAC_TXREMAINING);
	iowrite32be(0, CSR_MINIMAC_SETUP);

	return 0;
}

static int minimac_rx(struct net_device *dev, int budget)
{
	struct minimac *tp = netdev_priv(dev);
	struct sk_buff *skb;
	int size;
	int received = 0;
	int count;
	void *src;
	int i, state;

	while (netif_running(dev) && received < budget ) {
		count = 0;
		for (i = 1; i <= MAX_PACKET_RECEPTION_SLOT && received < budget ; ++i) {
			flush_dcache_range(CSR_MINIMAC_STATE0, 256);
			state = ioread32be(CSR_MINIMAC_STATE0+(i-1)*12);
			if (state & MINIMAC_STATE_PENDING) {
				size = ioread32be(CSR_MINIMAC_COUNT0+(i-1)*12);
				if (size == 0) {
				} else if (size < 64) {
					dev_dbg(&dev->dev, "rx: frame too short(%d)\n", size);

					dev->stats.rx_errors++;
					dev->stats.rx_length_errors++;
					++received;
					++count;
				} else if (size > MAX_ETH_FRAME_SIZE ) {
					dev_dbg(&dev->dev, "rx: frame too long(%d)\n", size);

					dev->stats.rx_errors++;
					dev->stats.rx_length_errors++;
					++received;
					++count;
				} else {
 					src = phys_to_virt(tp->pkt_buf[i].buf);
					size = get_frame(src, size );
					if (size > 0) {
						skb = netdev_alloc_skb_ip_align(dev, size);
						if (likely(skb)) {
							skb_copy_to_linear_data (skb, src+8, size);
							skb_put(skb, size);
							skb->protocol = eth_type_trans(skb, dev);
							dev->stats.rx_packets++;
							dev->stats.rx_bytes += size;
							netif_receive_skb(skb);
						} else {
							if (net_ratelimit())
								dev_warn(&dev->dev, "Memory squeeze, dropping packet\n");
							dev->stats.rx_dropped++;
						}
					} else if (size == -2) {
						dev->stats.rx_errors++;
						dev->stats.rx_crc_errors++;
						dev_dbg(&dev->dev, "rx: wrong CRC\n");

					}
					++received;
					++count;
				}
				iowrite32be(MINIMAC_STATE_LOADED, CSR_MINIMAC_STATE0+(i-1)*12);
			}
		}
		if (count == 0)
			break;
	}

	return received;
}

static void minimac_tx_timeout(struct net_device *dev)
{
	struct minimac *tp = netdev_priv(dev);

	if (netif_running(dev)) {
		minimac_reset(tp);
		netif_wake_queue(dev);
	}
}

static irqreturn_t minimac_interrupt_rx(int irq, void *dev_id)
{
	struct net_device *dev = (struct net_device *)dev_id;
	struct minimac *tp = netdev_priv(dev);

	flush_dcache_range(CSR_MINIMAC_SETUP, 256);
	if (ioread32be(CSR_MINIMAC_SETUP) & MINIMAC_SETUP_RXRST)
		iowrite32be(0, CSR_MINIMAC_SETUP);

	disable_irq_nosync(dev->irq);

	napi_schedule(&tp->napi);

	return IRQ_HANDLED;
}

static irqreturn_t minimac_interrupt_tx(int irq, void *dev_id)
{
	struct net_device *dev = (struct net_device *)dev_id;
	struct minimac *tp = netdev_priv(dev);

	spin_lock(&tp->lock);

	dev->stats.tx_bytes += (tp->pkt_buf)->count;
	dev->stats.tx_packets++;

	netif_wake_queue(dev);

	spin_unlock(&tp->lock);

	return IRQ_HANDLED;
}

static int minimac_poll(struct napi_struct *napi, int budget)
{
	struct minimac *tp = container_of(napi, struct minimac, napi);
	struct net_device *dev = tp->netdev;
	int work_done;

	work_done = minimac_rx(dev, budget);

	if (work_done < budget) {
		napi_complete(napi);
		enable_irq(dev->irq);
	}

	return work_done;
}

static int minimac_open(struct net_device *dev)
{
	struct minimac *tp = netdev_priv(dev);
	int ret;

	ret = request_irq(dev->irq, minimac_interrupt_rx, 0, "milkymist_minimac RX", dev);
	if (ret)
		return ret;

	ret = request_irq((dev->irq)+1, minimac_interrupt_tx, 0, "milkymist_minimac TX", dev);
	if (ret) {
		free_irq(dev->irq, dev);
		return ret;
	}

	minimac_reset(tp);
	netif_start_queue(dev);
	napi_enable(&tp->napi);

	return 0;
}

static int minimac_stop(struct net_device *dev)
{
	struct minimac *tp = netdev_priv(dev);
	int i;

	napi_disable(&tp->napi);

	netif_stop_queue(dev);

	free_irq(dev->irq, dev);
	free_irq(dev->irq+1, dev);

	for (i = 0; i < MAX_PACKET_RECEPTION_SLOT; ++i)
		iowrite32be(MINIMAC_STATE_EMPTY, CSR_MINIMAC_STATE0+i*12);

	iowrite32be(MINIMAC_SETUP_RXRST | MINIMAC_SETUP_TXRST, CSR_MINIMAC_SETUP);
	iowrite32be(0, CSR_MINIMAC_TXREMAINING);

	return 0;
}

static struct net_device_stats *minimac_stats(struct net_device *dev)
{
	return &dev->stats;
}

static netdev_tx_t minimac_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct minimac *tp = netdev_priv(dev);
	unsigned char *dest;
	unsigned long fcs;
	int len;

	if (ioread32be(CSR_MINIMAC_TXREMAINING) != 0)
		return NETDEV_TX_BUSY;

	spin_lock_irq(&tp->lock);

	len = skb->len;

	dest = phys_to_virt(tp->pkt_buf->buf);
	memcpy_toio(dest+8, skb->data, len);

	if (len < 60)
		len = 60;

	/* CRC-32 */
	fcs = crc32(dest + 8, len);
	(dest+len)[8] = (fcs & 0xff);
	(dest+len)[9] = (fcs & 0xff00) >> 8;
	(dest+len)[10] = (fcs & 0xff0000) >> 16;
	(dest+len)[11] = (fcs & 0xff000000) >> 24;

	iowrite32be(dest, CSR_MINIMAC_TXADR);
	iowrite32be(len + 12, CSR_MINIMAC_TXREMAINING);
	(tp->pkt_buf)->count = len;

	netif_stop_queue(dev);

	dev_kfree_skb(skb);
	dev->trans_start = jiffies;
	spin_unlock_irq(&tp->lock);

	return NETDEV_TX_OK;
}

static const struct net_device_ops minimac_netdev_ops = {
	.ndo_open	= minimac_open,
	.ndo_stop	= minimac_stop,
	.ndo_tx_timeout	= minimac_tx_timeout,
	.ndo_get_stats	= minimac_stats,
	.ndo_start_xmit	= minimac_start_xmit,
};

static int minimac_probe(struct platform_device *pdev)
{
	struct net_device *netdev = NULL;
	struct resource *res = NULL;
	struct resource *mmio = NULL;
	struct minimac *tp = NULL;
	static int first = 1;
	int ret = 0;
	unsigned char macadr[] = {0x10, 0xe2, 0xd5, 0x00, 0x00, 0x01};

	/* allocate networking device */
	netdev = alloc_etherdev(sizeof(struct minimac));
	if (!netdev) {
		dev_err(&pdev->dev, "cannot allocate network device\n");
		ret = -ENOMEM;
		goto out;
	}

	SET_NETDEV_DEV(netdev, &pdev->dev);
	platform_set_drvdata(pdev, netdev);

	/* obtain I/O memory space */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "cannot obtain I/O memory space\n");
		ret = -ENXIO;
		goto free;
	}

	mmio = devm_request_mem_region(&pdev->dev, res->start,
			resource_size(res), res->name);
	if (!mmio) {
		dev_err(&pdev->dev, "cannot request I/O memory space\n");
		ret = -ENXIO;
		goto free;
	}

	netdev->base_addr = mmio->start;

	/* obtain buffer memory space */
	netdev->mem_start = (unsigned long)kmalloc(buffer_size, GFP_KERNEL | GFP_DMA);
	if (!netdev->mem_start) {
		dev_err(&pdev->dev, "cannot allocate memory space\n");
		ret = -ENXIO;
		goto free;
	}
	netdev->mem_end = netdev->mem_start + buffer_size;

	/* obtain device IRQ number */
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		dev_err(&pdev->dev, "cannot obtain IRQ\n");
		ret = -ENXIO;
		goto free;
	}

	netdev->irq = res->start;

	tp = netdev_priv(netdev);
	tp->netdev = netdev;
	tp->membase = (void *)netdev->mem_start;

	tp->iobase = devm_ioremap_nocache(&pdev->dev, netdev->base_addr,
			resource_size(mmio));
	if (!tp->iobase) {
		dev_err(&pdev->dev, "cannot remap I/O memory space\n");
		ret = -ENXIO;
		goto error;
	}

	/* GPIO switch*/
	macadr[5] = ioread32be(CSR_GPIO_IN) >> 5;
	/* store MAC address */
	memcpy(netdev->dev_addr, macadr, IFHWADDRLEN);

	if (first) {
		printk(KERN_INFO "minimac: HWaddr %02x:%02x:%02x:%02x:%02x:%02x\n",
		macadr[0], macadr[1], macadr[2], macadr[3], macadr[4], macadr[5] );
		first = 0;
	}

	ether_setup(netdev);

	/* net_device */
	netdev->netdev_ops     = &minimac_netdev_ops;
	netdev->features       = 0;
	netdev->watchdog_timeo = WATCHDOG_TIMEOUT;

	/* setup NAPI */
	memset(&tp->napi, 0, sizeof(tp->napi));
	netif_napi_add(netdev, &tp->napi, minimac_poll, 64);

	/* reset stats */
	memset(&netdev->stats, 0, sizeof(netdev->stats));

	spin_lock_init(&tp->lock);
	spin_lock_init(&tp->rx_lock);

	ret = register_netdev(netdev);
	if (ret < 0) {
		dev_err(&netdev->dev, "failed to register interface\n");
		goto error;
	}

	goto out;

error:
free:
	if (tp->membase) {
		kfree(tp->membase);
		tp->membase = NULL;
	}
	free_netdev(netdev);
out:
	return ret;
}

static int minimac_remove(struct platform_device *pdev)
{
	struct net_device *netdev = platform_get_drvdata(pdev);
	struct minimac *tp = netdev_priv(netdev);

	platform_set_drvdata(pdev, NULL);

	if (netdev) {

		if (tp->membase) {
			kfree(tp->membase);
			tp->membase = NULL;
		}
		unregister_netdev(netdev);
		free_netdev(netdev);
	}

	return 0;
}

static struct platform_driver minimac_driver = {
	.driver  = {
		.name = "minimac",
	},
	.probe   = minimac_probe,
	.remove  = minimac_remove,
};

static int __init minimac_init(void)
{
	return platform_driver_register(&minimac_driver);
}

static void __exit minimac_exit(void)
{
	platform_driver_unregister(&minimac_driver);
}

module_init(minimac_init);
module_exit(minimac_exit);

MODULE_AUTHOR("");
MODULE_DESCRIPTION("Milkymist minimac driver");
MODULE_LICENSE("");

