/*
 *  Copyright (C) 2009-2010, Lars-Peter Clausen <lars@metafoo.de>
 *	JZ4740 SoC LCD framebuffer driver
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
#include <linux/platform_device.h>

#include <linux/io.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>

#include <asm/hw/milkymist.h>

/*
 *  RAM we reserve for the frame buffer. This defines the maximum screen
 *  size
 *
 *  The default can be overridden if the driver is compiled as a module
 */
#define VIDMEMSIZE	(1024*768*2)

struct milkymistfb {
	struct fb_info *fb;

	void *vidmem;
	dma_addr_t vidmem_phys;

	uint32_t pseudo_palette[16];
};

static unsigned long videomemorysize = VIDMEMSIZE;
module_param(videomemorysize, ulong, 0);

static char *milkymistfb_mode_option = "640x480@60";

static const struct fb_videomode milkymist_modedb[] = {
	{ NULL, 60, 640, 480, 40000, 47, 16, 32, 30, 96, 2,	0,
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

static int milkymistfb_enable __initdata;	/* disabled by default */
module_param(milkymistfb_enable, bool, 0);

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

	line_length = get_line_length(var->xres_virtual, var->bits_per_pixel);
	if (line_length * var->yres_virtual > videomemorysize)
		return -ENOMEM;

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

	iowrite32be(VGA_RESET, CSR_VGA_RESET);
	iowrite32be(milkymistfb->vidmem_phys, CSR_VGA_BASEADDRESS);
	iowrite32be(info->var.xres, CSR_VGA_HRES);
	iowrite32be(hsync_start, CSR_VGA_HSYNC_START);
	iowrite32be(hsync_end, CSR_VGA_HSYNC_END);
	iowrite32be(hscan, CSR_VGA_HSCAN);
	iowrite32be(info->var.yres, CSR_VGA_VRES);
	iowrite32be(vsync_start, CSR_VGA_VSYNC_START);
	iowrite32be(vsync_end, CSR_VGA_VSYNC_END);
	iowrite32be(vscan, CSR_VGA_VSCAN);
	iowrite32be(burst_count, CSR_VGA_BURST_COUNT);

	if (info->var.pixclock >= 39000) /* 25 MHz */
		iowrite32be(0, CSR_VGA_CLKSEL);
	else if (info->var.pixclock >= 19000) /* 50 MHz */
		iowrite32be(1, CSR_VGA_CLKSEL);
	else /* 65 MHz */
		iowrite32be(2, CSR_VGA_CLKSEL);

	iowrite32be(0, CSR_VGA_RESET);

	return 0;
}

/* Based on CNVT_TOHW macro from skeletonfb.c */
static inline uint32_t milkymistfb_convert_color_to_hw(unsigned val,
	struct fb_bitfield *bf)
{
	return (((val << bf->length) + 0x7FFF - val) >> 16) << bf->offset;
}


static int milkymistfb_setcolreg(unsigned regno, unsigned red, unsigned green,
	unsigned blue, unsigned transp, struct fb_info *info)
{
	uint32_t color;

	if (regno >= 16)
		return -EINVAL;

	color = milkymistfb_convert_color_to_hw(red, &info->var.red);
	color |= milkymistfb_convert_color_to_hw(green, &info->var.green);
	color |= milkymistfb_convert_color_to_hw(blue, &info->var.blue);
	color |= milkymistfb_convert_color_to_hw(transp, &info->var.transp);

	((uint32_t *)(info->pseudo_palette))[regno] = color;

	return 0;
}

/* Based on bf54x-lq043fb.c */
static int milkymistfb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	struct milkymistfb *milkymistfb = info->par;

	vma->vm_start = (unsigned int)milkymistfb->vidmem;
	vma->vm_end = vma->vm_start + info->fix.smem_len;
	vma->vm_flags |= VM_MAYSHARE | VM_SHARED;

	return 0;
}

#ifndef MODULE
static int __init milkymistfb_setup(char *options)
{
	char *this_opt;

	milkymistfb_enable = 1;

	if (!options || !*options)
		return 1;

	while ((this_opt = strsep(&options, ",")) != NULL) {
		if (!*this_opt)
			continue;
		if (!strncmp(this_opt, "mode:", 5))
			milkymistfb_mode_option = this_opt + 5;
		else if (!strncmp(this_opt, "disable", 7))
			milkymistfb_enable = 0;
	}
	return 1;
}
#endif  /*  MODULE  */

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
	.fb_mmap	= milkymistfb_mmap,
};

static int __devinit milkymistfb_probe(struct platform_device *pdev)
{
	struct milkymistfb *milkymistfb;
	struct fb_info *info;
	int ret = -ENOMEM;

	info = framebuffer_alloc(sizeof(*milkymistfb), &pdev->dev);
	if (!info)
		return -ENOMEM;

	info->fbops = &milkymistfb_ops;
	milkymistfb = info->par;
	milkymistfb->fb = info;

	fb_find_mode(&info->var, info, milkymistfb_mode_option,
		milkymist_modedb, ARRAY_SIZE(milkymist_modedb), NULL, 16);

/*	milkymistfb->vidmem = dma_alloc_coherent(&pdev->dev, videomemorysize,
		&milkymistfb->vidmem_phys, GFP_KERNEL);*/
	milkymistfb->vidmem = vmalloc(videomemorysize);
	milkymistfb->vidmem_phys = (unsigned int)milkymistfb->vidmem;
	if (!milkymistfb->vidmem) {
		ret = -ENOMEM;
		goto err_framebuffer_release;
	}
	memset(milkymistfb->vidmem, 0, videomemorysize);

	info->screen_base = milkymistfb->vidmem;
	info->fix = milkymistfb_fix;
	info->pseudo_palette = milkymistfb->pseudo_palette;
	info->flags = FBINFO_FLAG_DEFAULT;
	info->fix.smem_start = milkymistfb->vidmem_phys;
	info->fix.smem_len = videomemorysize;

	ret = fb_alloc_cmap(&info->cmap, 256, 0);
	if (ret < 0)
		goto err_dma_free;

	ret = register_framebuffer(info);
	if (ret < 0)
		goto err_dealloc_cmap;

	platform_set_drvdata(pdev, milkymistfb);

	dev_info(&pdev->dev,
	       "fb%d: Milkymist frame buffer at %p, size %ld k\n",
	       info->node, milkymistfb->vidmem, videomemorysize >> 10);

	return 0;

err_dealloc_cmap:
	fb_dealloc_cmap(&info->cmap);
err_dma_free:
/*	dma_free_coherent(&pdev->dev, videomemorysize, milkymistfb->vidmem,
		milkymistfb->vidmem_phys);*/
	vfree(milkymistfb->vidmem);
err_framebuffer_release:
	framebuffer_release(info);
	return ret;
}

static int __devexit milkymistfb_remove(struct platform_device *pdev)
{
	struct milkymistfb *milkymistfb = platform_get_drvdata(pdev);
	struct fb_info *info = milkymistfb->fb;

	iowrite32be(VGA_RESET, CSR_VGA_RESET);

	unregister_framebuffer(info);
	framebuffer_release(info);
	/*
	dma_free_coherent(&pdev->dev, videomemorysize, milkymistfb->vidmem,
		milkymistfb->vidmem_phys);*/
	vfree(milkymistfb->vidmem);
	fb_dealloc_cmap(&info->cmap);

	platform_set_drvdata(pdev, NULL);

	return 0;
}

static struct platform_driver milkymistfb_driver = {
	.probe	= milkymistfb_probe,
	.remove = __devexit_p(milkymistfb_remove),
	.driver = {
		.name	= "milkymistfb",
		.owner	= THIS_MODULE,
	},
};

static struct platform_device *milkymistfb_device;

static int __init milkymistfb_init(void)
{
	int ret = 0;

#ifndef MODULE
	char *option = NULL;

	if (fb_get_options("milkymistfb", &option))
		return -ENODEV;
	milkymistfb_setup(option);
#endif

	if (!milkymistfb_enable)
		return -ENXIO;

	ret = platform_driver_register(&milkymistfb_driver);

	if (!ret) {
		milkymistfb_device = platform_device_alloc("milkymistfb", 0);

		if (milkymistfb_device)
			ret = platform_device_add(milkymistfb_device);
		else
			ret = -ENOMEM;

		if (ret) {
			platform_device_put(milkymistfb_device);
			platform_driver_unregister(&milkymistfb_driver);
		}
	}

	return ret;
}
module_init(milkymistfb_init);

static void __exit milkymistfb_exit(void)
{
	platform_device_unregister(milkymistfb_device);
	platform_driver_unregister(&milkymistfb_driver);
}
module_exit(milkymistfb_exit);

MODULE_LICENSE("GPL");
