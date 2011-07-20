/*
 * linux/drivers/net/milkymist_minimac.c
 */

#include <linux/etherdevice.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/crc32.h>

#include <asm/cacheflush.h>
#include <asm/hw/milkymist.h>

static int buffer_size = 0x4000;	/* 16KBytes */
module_param(buffer_size, int, 0);
MODULE_PARM_DESC(buffer_size, "DMA buffer allocation size");

#define	MAX_PACKET_RECEPTION_SLOT	4
#define	MAX_ETH_FRAME_SIZE		1536
#define	WATCHDOG_TIMEOUT		(5 * HZ)

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

	computed_crc = crc32(0, src+8, len-12);

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

	iowrite32be(MINIMAC_SETUP_RXRST | MINIMAC_SETUP_TXRST, dev->iobase + CSR_MINIMAC_SETUP);

	for (i = 0; i < MAX_PACKET_RECEPTION_SLOT; ++i) {
		iowrite32be((unsigned int)dev->pkt_buf[i].buf, dev->iobase + CSR_MINIMAC_ADDR0+i*12);
		iowrite32be(MINIMAC_STATE_LOADED, dev->iobase + CSR_MINIMAC_STATE0+i*12);
	}

	memcpy_toio(dev->pkt_buf->buf, preamble, 8);		/* TX buff preamble */

	iowrite32be(0, dev->iobase + CSR_MINIMAC_TXREMAINING);
	iowrite32be(0, dev->iobase + CSR_MINIMAC_SETUP);

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
			flush_dcache_range(tp->iobase + CSR_MINIMAC_STATE0, 256);
			state = ioread32be(tp->iobase + CSR_MINIMAC_STATE0+(i-1)*12);
			if (state & MINIMAC_STATE_PENDING) {
				size = ioread32be(tp->iobase + CSR_MINIMAC_COUNT0+(i-1)*12);
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
				iowrite32be(MINIMAC_STATE_LOADED, tp->iobase + CSR_MINIMAC_STATE0+(i-1)*12);
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
	if (ioread32be(tp->iobase + CSR_MINIMAC_SETUP) & MINIMAC_SETUP_RXRST)
		iowrite32be(0, tp->iobase + CSR_MINIMAC_SETUP);

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
		iowrite32be(MINIMAC_STATE_EMPTY, tp->iobase + CSR_MINIMAC_STATE0+i*12);

	iowrite32be(MINIMAC_SETUP_RXRST | MINIMAC_SETUP_TXRST, tp->iobase + CSR_MINIMAC_SETUP);
	iowrite32be(0, tp->iobase + CSR_MINIMAC_TXREMAINING);

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

	if (ioread32be(tp->iobase + CSR_MINIMAC_TXREMAINING) != 0)
		return NETDEV_TX_BUSY;

	spin_lock_irq(&tp->lock);

	len = skb->len;

	dest = phys_to_virt(tp->pkt_buf->buf);
	memcpy_toio(dest+8, skb->data, len);

	if (len < 60)
		len = 60;

	/* CRC-32 */
	fcs = crc32(0, dest + 8, len);
	(dest+len)[8] = (fcs & 0xff);
	(dest+len)[9] = (fcs & 0xff00) >> 8;
	(dest+len)[10] = (fcs & 0xff0000) >> 16;
	(dest+len)[11] = (fcs & 0xff000000) >> 24;

	iowrite32be(dest, tp->iobase + CSR_MINIMAC_TXADR);
	iowrite32be(len + 12, tp->iobase + CSR_MINIMAC_TXREMAINING);
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

static const struct of_device_id minimac_of_ids[] = {
	{ .compatible = "milkymist,minimac", },
	{}
};

static struct platform_driver minimac_driver = {
	.driver  = {
		.name = "minimac",
		.of_match_table = minimac_of_ids,
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

