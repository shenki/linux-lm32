/*
 *  Copyright (C) 2009-2011, Lars-Peter Clausen <lars@metafoo.de>
 *  Copyright (C) 2011, Michael Walle <michael@walle.cc>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of  the GNU General Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/interrupt.h>

#include <linux/io.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>

#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>

#define DRIVER_NAME  "milkymist_fb"

/* hardware registers */
#define VGAFB_CTRL             0x00
#define VGAFB_HRES             0x04
#define VGAFB_HSYNC_START      0x08
#define VGAFB_HSYNC_END        0x0c
#define VGAFB_HSCAN            0x10
#define VGAFB_VRES             0x14
#define VGAFB_VSYNC_START      0x18
#define VGAFB_VSYNC_END        0x1c
#define VGAFB_VSCAN            0x20
#define VGAFB_BASEADDRESS      0x24
#define VGAFB_BASEADDRESS_ACT  0x28
#define VGAFB_BURST_COUNT      0x2c
#define VGAFB_DDC              0x30
#define VGAFB_CLKSEL           0x34
#define REG_MAX                0x38

#define CTRL_RESET       (1<<0)

#define CLKSEL_25MHZ     0
#define CLKSEL_50MHZ     1
#define CLKSEL_65MHZ     2

#define DDC_SDA_IN       (1<<0)
#define DDC_SDA_OUT      (1<<1)
#define DDC_SDA_OE       (1<<2)
#define DDC_SDA_SDC      (1<<3)

struct milkymistfb {
	struct fb_info *fb;

	void *vidmem_virt;
	unsigned long vidmem_phys;
	unsigned char __iomem *ctrlbase;

	uint32_t pseudo_palette[16];
};

static char *mode_option = "640x480@60";

static const struct fb_videomode milkymist_modedb[] = {
	{ NULL, 60, 640, 480, 40000, 47, 16, 32, 30, 96, 2, 0,
		FB_VMODE_NONINTERLACED },
	{ NULL, 72, 800, 600, 20000, 80, 32, 23, 37, 128, 6, 0,
		FB_VMODE_NONINTERLACED },
	{ NULL, 60, 1024, 768, 15384, 168, 8, 29, 3, 144, 6, 0,
		FB_VMODE_NONINTERLACED },
};

static const struct fb_fix_screeninfo milkymistfb_fix = {
	.id =		"MilkymistFB",
	.type =		FB_TYPE_PACKED_PIXELS,
	.visual =	FB_VISUAL_TRUECOLOR,
	.accel =	FB_ACCEL_NONE,
};

static unsigned int get_line_length(unsigned int xres_virtual, unsigned int bpp)
{
	unsigned int length;

	length = xres_virtual * bpp;
	length = (length + 31) & ~31;
	length >>= 3;
	return length;
}

static int milkymistfb_check_var(struct fb_var_screeninfo *var,
			 struct fb_info *info)
{
	u_long line_length;

	if (var->bits_per_pixel != 16)
		return -EINVAL;

	if (!var->xres)
		var->xres = 1;
	if (!var->yres)
		var->yres = 1;
	if (var->xres > var->xres_virtual)
		var->xres_virtual = var->xres;
	if (var->yres > var->yres_virtual)
		var->yres_virtual = var->yres;

	if (var->rotate) {
		dev_dbg(info->device, "Rotation is not supported\n");
		return -EINVAL;
	}

	line_length = get_line_length(var->xres_virtual, var->bits_per_pixel);
	if (line_length * var->yres_virtual > info->fix.smem_len) {
		dev_dbg(info->device, "Not enough memory\n");
		return -ENOMEM;
	}

	var->red.offset = 11;
	var->red.length = 5;
	var->green.offset = 5;
	var->green.length = 6;
	var->blue.offset = 0;
	var->blue.length = 5;
	var->transp.offset = 0;
	var->transp.length = 0;
	var->red.msb_right = 0;
	var->green.msb_right = 0;
	var->blue.msb_right = 0;
	var->transp.msb_right = 0;

	return 0;
}

static int milkymistfb_set_par(struct fb_info *info)
{
	struct milkymistfb *milkymistfb = info->par;
	unsigned int hsync_start, hsync_end, hscan;
	unsigned int vsync_start, vsync_end, vscan;
	unsigned int burst_count;

	info->fix.line_length = get_line_length(info->var.xres_virtual,
						info->var.bits_per_pixel);

	hsync_start = info->var.xres + info->var.right_margin;
	hsync_end = hsync_start + info->var.hsync_len;
	hscan = hsync_end + info->var.left_margin;

	vsync_start = info->var.yres + info->var.lower_margin;
	vsync_end = vsync_start + info->var.vsync_len;
	vscan = vsync_end + info->var.upper_margin;

	burst_count = (info->var.xres * info->var.yres * 16) / (64*4);

	/* assert reset */
	iowrite32be(CTRL_RESET, milkymistfb->ctrlbase + VGAFB_CTRL);

	iowrite32be(milkymistfb->vidmem_phys,
			milkymistfb->ctrlbase + VGAFB_BASEADDRESS);
	iowrite32be(info->var.xres, milkymistfb->ctrlbase + VGAFB_HRES);
	iowrite32be(hsync_start, milkymistfb->ctrlbase + VGAFB_HSYNC_START);
	iowrite32be(hsync_end, milkymistfb->ctrlbase + VGAFB_HSYNC_END);
	iowrite32be(hscan, milkymistfb->ctrlbase + VGAFB_HSCAN);
	iowrite32be(info->var.yres, milkymistfb->ctrlbase + VGAFB_VRES);
	iowrite32be(vsync_start, milkymistfb->ctrlbase + VGAFB_VSYNC_START);
	iowrite32be(vsync_end, milkymistfb->ctrlbase + VGAFB_VSYNC_END);
	iowrite32be(vscan, milkymistfb->ctrlbase + VGAFB_VSCAN);
	iowrite32be(burst_count, milkymistfb->ctrlbase + VGAFB_BURST_COUNT);

	if (info->var.pixclock >= 39000)
		iowrite32be(CLKSEL_25MHZ, milkymistfb->ctrlbase + VGAFB_CLKSEL);
	else if (info->var.pixclock >= 19000)
		iowrite32be(CLKSEL_50MHZ, milkymistfb->ctrlbase + VGAFB_CLKSEL);
	else
		iowrite32be(CLKSEL_65MHZ, milkymistfb->ctrlbase + VGAFB_CLKSEL);

	/* take hardware out of reset */
	iowrite32be(0, milkymistfb->ctrlbase + VGAFB_CTRL);

	return 0;
}

/* Based on CNVT_TOHW macro from skeletonfb.c */
static inline uint32_t milkymistfb_convert_color_to_hw(unsigned int val,
	struct fb_bitfield *bf)
{
	return (((val << bf->length) + 0x7fff - val) >> 16) << bf->offset;
}


static int milkymistfb_setcolreg(unsigned int regno,
		unsigned int red, unsigned int green, unsigned int blue,
		unsigned int transp, struct fb_info *info)
{
	struct milkymistfb *milkymistfb = info->par;
	uint32_t color;

	if (regno >= 16)
		return -EINVAL;

	color = milkymistfb_convert_color_to_hw(red, &info->var.red);
	color |= milkymistfb_convert_color_to_hw(green, &info->var.green);
	color |= milkymistfb_convert_color_to_hw(blue, &info->var.blue);
	color |= milkymistfb_convert_color_to_hw(transp, &info->var.transp);

	milkymistfb->pseudo_palette[regno] = color;

	return 0;
}

#if 0
/* Based on bf54x-lq043fb.c */
static int milkymistfb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	struct milkymistfb *milkymistfb = info->par;

	vma->vm_start = (unsigned int)milkymistfb->vidmem_virt;
	vma->vm_end = vma->vm_start + info->fix.smem_len;
	vma->vm_flags |= VM_MAYSHARE | VM_SHARED;

	return 0;
}
#endif

static struct fb_ops milkymistfb_ops = {
	.owner		= THIS_MODULE,
	.fb_read	= fb_sys_read,
	.fb_write	= fb_sys_write,
	.fb_check_var	= milkymistfb_check_var,
	.fb_set_par	= milkymistfb_set_par,
	.fb_setcolreg	= milkymistfb_setcolreg,
	.fb_fillrect	= sys_fillrect,
	.fb_copyarea	= sys_copyarea,
	.fb_imageblit	= sys_imageblit,
#if 0
	.fb_mmap	= milkymistfb_mmap,
#endif
};

static int milkymistfb_probe(struct platform_device *ofdev)
{
	struct device_node *np = ofdev->dev.of_node;
	struct fb_info *info;
	struct milkymistfb *milkymistfb;
	int ret;
	struct resource res;
	const unsigned int *vidmemsize;

	ret = of_address_to_resource(np, 0, &res);
	if (ret) {
		dev_err(&ofdev->dev, "invalid address\n");
		return ret;
	}

	/*  RAM we reserve for the frame buffer. This defines the maximum
	 *  screen size */
	vidmemsize = of_get_property(np, "video-mem-size", NULL);
	if (!vidmemsize) {
		dev_err(&ofdev->dev, "no video-mem-size property set\n");
		return -ENODEV;
	}

	info = framebuffer_alloc(sizeof(struct milkymistfb), &ofdev->dev);
	if (!info)
		return -ENOMEM;

	milkymistfb = info->par;
	milkymistfb->fb = info;

	milkymistfb->ctrlbase = ioremap(res.start, REG_MAX);

	info->fbops = &milkymistfb_ops;

	/* allocate framebuffer memory */
	milkymistfb->vidmem_virt = vmalloc(*vidmemsize);
	milkymistfb->vidmem_phys = virt_to_phys(milkymistfb->vidmem_virt);

	if (!milkymistfb->vidmem_virt) {
		dev_err(&ofdev->dev, "could not allocate framebuffer\n");
		ret = -ENOMEM;
		goto err_framebuffer_release;
	}
	memset(milkymistfb->vidmem_virt, 0, *vidmemsize);

	info->screen_base = milkymistfb->vidmem_virt;
	info->fix = milkymistfb_fix;
	info->pseudo_palette = milkymistfb->pseudo_palette;
	info->flags = FBINFO_FLAG_DEFAULT;
	info->fix.smem_start = milkymistfb->vidmem_phys;
	info->fix.smem_len = *vidmemsize;

	fb_find_mode(&info->var, info, mode_option,
		milkymist_modedb, ARRAY_SIZE(milkymist_modedb), NULL, 16);

	ret = fb_alloc_cmap(&info->cmap, 256, 0);
	if (ret < 0)
		goto err_dma_free;

	ret = register_framebuffer(info);
	if (ret < 0)
		goto err_dealloc_cmap;

	platform_set_drvdata(ofdev, milkymistfb);

	dev_info(&ofdev->dev,
	       "fb%d: Milkymist frame buffer at %p, size %ukB\n",
	       info->node, milkymistfb->vidmem_virt, *vidmemsize >> 10);

	return 0;

err_dealloc_cmap:
	fb_dealloc_cmap(&info->cmap);
err_dma_free:
	vfree(milkymistfb->vidmem_virt);
err_framebuffer_release:
	framebuffer_release(info);
	return ret;
}

static int milkymistfb_remove(struct platform_device *ofdev)
{
	struct milkymistfb *milkymistfb = platform_get_drvdata(ofdev);
	struct fb_info *info = milkymistfb->fb;

	/* take hardware into reset */
	iowrite32be(CTRL_RESET, milkymistfb->ctrlbase + VGAFB_CTRL);

	unregister_framebuffer(info);

	fb_dealloc_cmap(&info->cmap);
	vfree(milkymistfb->vidmem_virt);
	framebuffer_release(info);

	platform_set_drvdata(ofdev, NULL);

	return 0;
}

static const struct of_device_id milkymist_vgafb_match[] = {
	{ .compatible = "milkymist,vgafb", },
	{},
};
MODULE_DEVICE_TABLE(of, milkymist_vgafb_match);

static struct platform_driver milkymist_vgafb_of_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table = milkymist_vgafb_match,
	},
	.probe		= milkymistfb_probe,
	.remove		= milkymistfb_remove,
};

#ifndef MODULE
static int __init milkymistfb_setup(char *options)
{
	char *this_opt;

	if (!options || !*options)
		return 0;

	while ((this_opt = strsep(&options, ",")) != NULL) {
		if (!*this_opt)
			continue;
		if (!strncmp(this_opt, "mode:", 5))
			mode_option = this_opt + 5;
		else
			printk(KERN_ERR "milkymistfb: unknown parameter %s\n",
					this_opt);
	}
	return 0;
}
#endif  /*  MODULE  */

static int __init milkymist_vgafb_init(void)
{
#ifndef MODULE
	char *option = NULL;

	if (fb_get_options("milkymistfb", &option))
		return -ENODEV;
	milkymistfb_setup(option);
#endif

	return platform_driver_register(&milkymist_vgafb_of_driver);
}

static void __exit milkymist_vgafb_exit(void)
{
	platform_driver_unregister(&milkymist_vgafb_of_driver);
}

module_init(milkymist_vgafb_init);
module_exit(milkymist_vgafb_exit);

MODULE_AUTHOR("Milkymist Project");
MODULE_DESCRIPTION("Milkymist VGAFB driver");
MODULE_LICENSE("GPL");
