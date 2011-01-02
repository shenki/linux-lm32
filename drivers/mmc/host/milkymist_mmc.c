/*
 *  Copyright (C) 2011, Lars-Peter Clausen <lars@metafoo.de>
 *  Milkymist SD/MMC controller driver
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <linux/mmc/host.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/scatterlist.h>
#include <linux/crc7.h>
#include <linux/crc-ccitt.h>

#include <asm/cacheflush.h>
#include <linux/dma-mapping.h>

#define MILKYMIST_REG_MMC_CLK2XDIV	0x00
#define MILKYMIST_REG_MMC_ENABLE	0x04
#define MILKYMIST_REG_MMC_PENDING	0x08
#define MILKYMIST_REG_MMC_START		0x0C
#define MILKYMIST_REG_MMC_CMD		0x10
#define MILKYMIST_REG_MMC_DATA		0x14

#define MILKYMIST_MMC_ENABLE_CMD_TX 0x1
#define MILKYMIST_MMC_ENABLE_CMD_RX 0x2
#define MILKYMIST_MMC_ENABLE_DATA_TX 0x4
#define MILKYMIST_MMC_ENABLE_DATA_RX 0x8

#define MILKYMIST_MMC_PENDING_CMD_TX 0x1
#define MILKYMIST_MMC_PENDING_CMD_RX 0x2
#define MILKYMIST_MMC_PENDING_DATA_TX 0x4
#define MILKYMIST_MMC_PENDING_DATA_RX 0x8
#define MILKYMIST_MMC_PENDING_RX (MILKYMIST_MMC_PENDING_CMD_RX | MILKYMIST_MMC_PENDING_DATA_RX)

#define MILKYMIST_MMC_START_CMD_RX 0x1
#define MILKYMIST_MMC_START_DATA_RX 0x2

#define MILKYMIST_RATE 80000000

struct milkymist_mmc_host {
	struct mmc_host *mmc;
	struct platform_device *pdev;

	struct resource *mem;
	void __iomem *base;
};

static void get_crc_bytes(uint32_t word, uint8_t *bytes)
{
	unsigned int i, j;
	for (j = 0; j < 4; ++j)
		bytes[j] = 0;
	for (i = 0; i < 8; ++i) {
		for (j = 0; j < 4; ++j)
			bytes[j] |= ((word >> j) & 1) << (7 - i);
		word >>= 4;
	}
}

#if 0
static int milkymist_mmc_write_data(struct milkymist_mmc_host *host,
	struct mmc_data *data)
{
	struct sg_mapping_iter *miter = &host->miter;
	void __iomem *data_addr = host->base + MOLKYMIST_REG_MMC_DATA;
	uint32_t *buf;
	size_t i;
	uint32_t tmp;

	d = 0x0;

	while (sg_miter_next(miter)) {

		buf = miter->addr;
		i = miter->length / 4;
		while (i) {
				d |= *buf >> 28;
				writel(d, data_addr);
				d = (*buf & 0xf) << 28;
				++buf;
				--i;
		}
		d |= 0x0fffffff;
		writel(d, data_addr);
		data->bytes_xfered += miter->length;
	}
	sg_miter_stop(miter);

	return 0;
}
#endif

static void milkymist_mmc_clear_start(struct milkymist_mmc_host *host, uint32_t flags)
{
	writel(flags, host->base + MILKYMIST_REG_MMC_START);
}

static void milkymist_mmc_enable_xfer(struct milkymist_mmc_host *host, uint32_t flags)
{
	writel(flags, host->base + MILKYMIST_REG_MMC_ENABLE);
}

static int milkymist_mmc_read_response_and_data(struct milkymist_mmc_host *host,
				struct mmc_command *cmd, struct mmc_data *data)
{
	struct sg_mapping_iter miter;
	void __iomem *data_addr = host->base + MILKYMIST_REG_MMC_DATA;
	void __iomem *cmd_addr = host->base + MILKYMIST_REG_MMC_CMD;
	void __iomem *pending_addr = host->base + MILKYMIST_REG_MMC_PENDING;
	uint32_t *buf;
	uint32_t pending;
	uint8_t resp[6];
	size_t i, j, k, l;
	bool transfer_data = true;
	int ret = 0;
	unsigned int timeout;
	uint16_t crc[4] = {0, 0, 0, 0};
	uint32_t d;
	uint8_t tmp[4];

	writel(MILKYMIST_MMC_PENDING_RX, pending_addr);
	milkymist_mmc_clear_start(host, MILKYMIST_MMC_START_CMD_RX |
	MILKYMIST_MMC_START_DATA_RX);
	milkymist_mmc_enable_xfer(host, MILKYMIST_MMC_ENABLE_CMD_RX | MILKYMIST_MMC_ENABLE_DATA_RX);

	sg_miter_start(&miter, data->sg, data->sg_len, SG_MITER_TO_SG);
	transfer_data = sg_miter_next(&miter);
	j = miter.length / 4;
	buf = miter.addr;

	if (j % 4 != 0)
		printk("AAAAAAAAAAAAAAAHHHHHHHH\n");

	k = 0;
/*	printk("foo: %d %d\n", miter.length, transfer_data);*/

	while (transfer_data || (k >= 128) || i < 6) {
		timeout = 1000000;
		do {
			pending = readl(pending_addr);
		} while (!(pending & MILKYMIST_MMC_PENDING_RX) && --timeout);
		if (timeout == 0) {
			ret = -EIO;
			printk("timeout\n");
			break;
		}

		if (pending & MILKYMIST_MMC_PENDING_CMD_RX) {
			resp[i] = readl(cmd_addr) & 0xff;
			if (++i == 6)
				milkymist_mmc_enable_xfer(host, MILKYMIST_MMC_ENABLE_DATA_RX);
		}

		if (pending & MILKYMIST_MMC_PENDING_DATA_RX) {

			if (k < 128) {
				d = readl(data_addr);
				*buf++ = d;
				get_crc_bytes(d, tmp);
				for (l = 0; l < 4; ++l)
					crc[l] = crc_ccitt(crc[l], &tmp[l], 1);
				/*				printk("buf[%d] = %x\n", j, *(buf - 1));*/
				if (--j == 0) {
					data->bytes_xfered += miter.length;
					flush_dcache_page(miter.page);
					transfer_data = sg_miter_next(&miter);
					if (transfer_data) {
						j = miter.length / 4;
						buf = miter.addr;
					}
				}
				++k;
			} else {
				d = readl(data_addr);
/*				printk("crc[%d] = %x\n", k-128, readl(data_addr));*/
				get_crc_bytes(d, tmp);
				for (l = 0; l < 4; ++l)
					crc[l] = crc_ccitt(crc[l], &tmp[l], 1);
				if (k == 129) {
					for (k = 0; k < 4;  ++k) {
						/*printk("crc[%d] = %x\n", k, crc[k]);*/
						crc[k] = 0;
					}
					k = 0;
				} else
					++k;
			}

		}

		/* Clear pending bits */
		writel(pending, pending_addr);
	}

	timeout = 10000;
	do {
		pending = readl(pending_addr);
	} while (!(pending & MILKYMIST_MMC_PENDING_RX) && --timeout);

	cmd->resp[0] = (resp[1] << 24) | (resp[2] << 16) | (resp[3] << 8) | resp[4];

	sg_miter_stop(&miter);
	milkymist_mmc_enable_xfer(host, 0);

	return ret;
}

static int milkymist_mmc_read_response(struct milkymist_mmc_host *host,
	struct mmc_command *cmd)
{
	void __iomem *cmd_addr = host->base + MILKYMIST_REG_MMC_CMD;
	void __iomem *pending_addr = host->base + MILKYMIST_REG_MMC_PENDING;
	uint32_t pending;
	uint8_t resp[17];
	unsigned int i;
	unsigned int size;
	unsigned int timeout;
/*	printk("cmd 1 readback: %x\n", readl(host->base + MILKYMIST_REG_MMC_CMD));*/

	writel(MILKYMIST_MMC_PENDING_CMD_RX, pending_addr);
	milkymist_mmc_clear_start(host, MILKYMIST_MMC_START_CMD_RX);
	milkymist_mmc_enable_xfer(host, MILKYMIST_MMC_ENABLE_CMD_RX);

	if (cmd->flags & MMC_RSP_136)
		size = 17;
	else
		size = 6;

	for (i = 0; i < size; ++i) {
		timeout = 10000;
		do {
			pending = readl(pending_addr);
		} while (!(pending & MILKYMIST_MMC_PENDING_CMD_RX) && --timeout);
		if (timeout == 0)
			return -EIO;
		resp[i] = readl(cmd_addr) & 0xff;
		writel(pending, pending_addr);
	}

	if (cmd->flags & MMC_RSP_136) {
		cmd->resp[0] = (resp[1] << 24) | (resp[2] << 16) | (resp[3] << 8) | resp[4];
		cmd->resp[1] = (resp[5] << 24) | (resp[6] << 16) | (resp[7] << 8) | resp[8];
		cmd->resp[2] = (resp[9] << 24) | (resp[10] << 16) | (resp[11] << 8) | resp[12];
		cmd->resp[3] = (resp[13] << 24) | (resp[14] << 16) | (resp[15] << 8) | resp[16];
	} else {
		cmd->resp[0] = (resp[1] << 24) | (resp[2] << 16) | (resp[3] << 8) | resp[4];
		cmd->resp[1] = resp[5] << 24;
	}

	return 0;
}

static int milkymist_mmc_send_command(struct milkymist_mmc_host *host,
	struct mmc_command *cmd)
{
	void __iomem *cmd_addr = host->base + MILKYMIST_REG_MMC_CMD;
	void __iomem *pending_addr = host->base + MILKYMIST_REG_MMC_PENDING;
	uint32_t pending;
	uint8_t buf[6];
	unsigned int i;

	buf[0] = cmd->opcode | 0x40;
	buf[1] = (cmd->arg >> 24) & 0xff;
	buf[2] = (cmd->arg >> 16) & 0xff;
	buf[3] = (cmd->arg >> 8) & 0xff;
	buf[4] = cmd->arg & 0xff;
	buf[5] = (crc7(0, buf, 5) << 1) | 0x1;

	milkymist_mmc_enable_xfer(host, MILKYMIST_MMC_ENABLE_CMD_TX);

	for (i = 0; i < 6; ++i) {
		writel(buf[i], cmd_addr);
		do {
			pending = readl(pending_addr);
/*			printk("send_command pending: %x\n", pending);*/
		} while (pending & MILKYMIST_MMC_PENDING_CMD_TX);
	}

/*	printk("cmd readback: %x %x\n", buf[5], readl(host->base +
 *	MILKYMIST_REG_MMC_CMD));*/

	return 0;
}

static void milkymist_mmc_set_clock_rate(struct milkymist_mmc_host *host, int rate)
{
	uint32_t div = 0;

	div = MILKYMIST_RATE / rate;
	writel(div, host->base + MILKYMIST_REG_MMC_CLK2XDIV);
	printk("set clock rate: %d %d\n", rate, div);
}

static void milkymist_mmc_request(struct mmc_host *mmc, struct mmc_request *req)
{
	struct milkymist_mmc_host *host = mmc_priv(mmc);
	struct mmc_command *cmd = req->cmd;
	int ret;

 	ret = milkymist_mmc_send_command(host, cmd);
	if (ret)
		goto done;

	if (cmd->data) {
		if (cmd->data->flags & MMC_DATA_WRITE) {
			printk("WRITE NOT SUPPORTED YET\n");
/*			ret = -EINVAL;*/
		} else {
			ret = milkymist_mmc_read_response_and_data(host, req->cmd, req->data);
		}
	} else {
		ret = milkymist_mmc_read_response(host, req->cmd);
	}

	if (req->stop)
		ret = milkymist_mmc_send_command(host, req->stop);

done:
	req->cmd->error = ret;
	mmc_request_done(mmc, req);
}

static void milkymist_mmc_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct milkymist_mmc_host *host = mmc_priv(mmc);
	if (ios->clock)
		milkymist_mmc_set_clock_rate(host, ios->clock);

	switch (ios->power_mode) {
	case MMC_POWER_UP:
		break;
	case MMC_POWER_ON:
		break;
	default:
		break;
	}

	switch (ios->bus_width) {
	case MMC_BUS_WIDTH_1:
		break;
	case MMC_BUS_WIDTH_4:
		break;
	default:
		break;
	}
}

static int milkymist_mmc_get_ro(struct mmc_host *mmc)
{
	return 1;
}

static const struct mmc_host_ops milkymist_mmc_ops = {
	.request	= milkymist_mmc_request,
	.set_ios	= milkymist_mmc_set_ios,
	.get_ro = milkymist_mmc_get_ro,
};

static int __devinit milkymist_mmc_probe(struct platform_device* pdev)
{
	int ret;
	struct mmc_host *mmc;
	struct milkymist_mmc_host *host;

	mmc = mmc_alloc_host(sizeof(struct milkymist_mmc_host), &pdev->dev);
	if (!mmc) {
		dev_err(&pdev->dev, "Failed to alloc mmc host structure\n");
		return -ENOMEM;
	}

	host = mmc_priv(mmc);

	host->mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!host->mem) {
		ret = -ENOENT;
		dev_err(&pdev->dev, "Failed to get base platform memory\n");
		goto err_free_host;
	}

	host->mem = request_mem_region(host->mem->start,
					resource_size(host->mem), pdev->name);
	if (!host->mem) {
		ret = -EBUSY;
		dev_err(&pdev->dev, "Failed to request base memory region\n");
		goto err_free_host;
	}

	host->base = ioremap_nocache(host->mem->start, resource_size(host->mem));
	if (!host->base) {
		ret = -EBUSY;
		dev_err(&pdev->dev, "Failed to ioremap base memory\n");
		goto err_release_mem_region;
	}

	mmc->ops = &milkymist_mmc_ops;
	mmc->f_min = MILKYMIST_RATE / 0x7ff;
	mmc->f_max = MILKYMIST_RATE / 100;
	mmc->ocr_avail = MMC_VDD_32_33 | MMC_VDD_33_34;
	mmc->caps = MMC_CAP_4_BIT_DATA;

	mmc->max_blk_size = 0xffff;
	mmc->max_blk_count = 1;
	mmc->max_req_size = mmc->max_blk_size * mmc->max_blk_count;

	mmc->max_segs =  1;
	mmc->max_seg_size = mmc->max_req_size;

	host->mmc = mmc;
	host->pdev = pdev;

	platform_set_drvdata(pdev, host);
	ret = mmc_add_host(mmc);

	if (ret) {
		dev_err(&pdev->dev, "Failed to add mmc host: %d\n", ret);
		goto err_iounmap;
	}
	dev_info(&pdev->dev, "Milkymist SD/MMC card driver registered\n");

	return 0;

err_iounmap:
	iounmap(host->base);
err_release_mem_region:
	release_mem_region(host->mem->start, resource_size(host->mem));
err_free_host:
	platform_set_drvdata(pdev, NULL);
	mmc_free_host(mmc);

	return ret;
}

static int __devexit milkymist_mmc_remove(struct platform_device *pdev)
{
	struct milkymist_mmc_host *host = platform_get_drvdata(pdev);

	mmc_remove_host(host->mmc);

	iounmap(host->base);
	release_mem_region(host->mem->start, resource_size(host->mem));

	platform_set_drvdata(pdev, NULL);
	mmc_free_host(host->mmc);

	return 0;
}

static struct platform_driver milkymist_mmc_driver = {
	.probe = milkymist_mmc_probe,
	.remove = __devexit_p(milkymist_mmc_remove),
	.driver = {
		.name = "milkymist-mmc",
		.owner = THIS_MODULE,
	},
};

static int __init milkymist_mmc_init(void)
{
	return platform_driver_register(&milkymist_mmc_driver);
}
module_init(milkymist_mmc_init);

static void __exit milkymist_mmc_exit(void)
{
	platform_driver_unregister(&milkymist_mmc_driver);
}
module_exit(milkymist_mmc_exit);

MODULE_DESCRIPTION("Milkymist SD/MMC controller driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lars-Peter Clausen <lars@metafoo.de>");
