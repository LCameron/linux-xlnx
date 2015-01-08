/*
 * Xylon logiCVC frame buffer Open Firmware driver
 *
 * Copyright (C) 2014 Xylon d.o.o.
 * Author: Davor Joja <davor.joja@logicbricks.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>

#include "xylonfb_core.h"
#include "logicvc.h"

static void xylonfb_init_ctrl(struct xylonfb_data *data, struct device_node *dn,
			      u32 *ctrl)
{
	u32 ctrl_reg = (LOGICVC_CTRL_HSYNC | LOGICVC_CTRL_VSYNC |
			LOGICVC_CTRL_DATA_ENABLE);

	XYLONFB_DBG(INFO, "%s", __func__);

	if (of_property_read_bool(dn, "hsync-active-low") ||
	    (data->hw_flags & LOGICVC_CTRL_HSYNC_INVERT))
		ctrl_reg |= LOGICVC_CTRL_HSYNC_INVERT;
	if (of_property_read_bool(dn, "vsync-active-low") ||
	    (data->hw_flags & LOGICVC_CTRL_VSYNC_INVERT))
		ctrl_reg |= LOGICVC_CTRL_VSYNC_INVERT;
	if (of_property_read_bool(dn, "data-enable-active-low") ||
	    (data->hw_flags & LOGICVC_CTRL_DATA_ENABLE_INVERT))
		ctrl_reg |= LOGICVC_CTRL_DATA_ENABLE_INVERT;
	if (of_property_read_bool(dn, "pixel-data-invert") ||
	    (data->hw_flags & LOGICVC_CTRL_PIXEL_DATA_INVERT))
		ctrl_reg |= LOGICVC_CTRL_PIXEL_DATA_INVERT;
	if (of_property_read_bool(dn, "pixel-data-output-trigger-high") ||
	    (data->hw_flags & LOGICVC_CTRL_PIXEL_DATA_TRIGGER_INVERT))
		ctrl_reg |= LOGICVC_CTRL_PIXEL_DATA_TRIGGER_INVERT;

	*ctrl = ctrl_reg;
}

static int xylonfb_layer_set_format(struct xylonfb_layer_fix_data *fd)
{
	XYLONFB_DBG(INFO, "%s", __func__);

	switch (fd->type) {
	case LOGICVC_LAYER_ALPHA:
		fd->format = XYLONFB_FORMAT_A8;
		break;

	case LOGICVC_LAYER_RGB:
		switch (fd->bpp) {
		case 8:
			switch (fd->transparency) {
			case LOGICVC_ALPHA_CLUT_16BPP:
				fd->format = XYLONFB_FORMAT_C8;
				fd->format_clut = XYLONFB_FORMAT_CLUT_ARGB6565;
				break;
			case LOGICVC_ALPHA_CLUT_32BPP:
				fd->format = XYLONFB_FORMAT_C8;
				fd->format_clut = XYLONFB_FORMAT_CLUT_ARGB8888;
				break;
			case LOGICVC_ALPHA_LAYER:
				fd->format = XYLONFB_FORMAT_RGB332;
				break;
			default:
				return -EINVAL;
			}
			break;
		case 16:
			if (fd->transparency != LOGICVC_ALPHA_LAYER)
				return -EINVAL;

			fd->format = XYLONFB_FORMAT_RGB565;
			break;
		case 32:
			switch (fd->transparency) {
			case LOGICVC_ALPHA_LAYER:
				fd->format = XYLONFB_FORMAT_XRGB8888;
				break;
			case LOGICVC_ALPHA_PIXEL:
				fd->format = XYLONFB_FORMAT_ARGB8888;
				break;
			default:
				return -EINVAL;
			}
			break;
		}
		break;

	case LOGICVC_LAYER_YUV:
		switch (fd->bpp) {
		case 8:
			if (fd->transparency != LOGICVC_ALPHA_CLUT_32BPP)
				return -EINVAL;

			fd->format = XYLONFB_FORMAT_C8;
			fd->format_clut = XYLONFB_FORMAT_CLUT_AYUV8888;
			break;
		case 16:
			if (fd->transparency != LOGICVC_ALPHA_LAYER)
				return -EINVAL;

			fd->format = XYLONFB_FORMAT_YUYV;
			break;
		case 32:
			if (fd->transparency != LOGICVC_ALPHA_PIXEL)
				return -EINVAL;

			fd->format = XYLONFB_FORMAT_AYUV;
			break;
		}
		break;

	default:
		pr_err("unsupported layer type\n");
		return -EINVAL;
	}

	return 0;
}

static int xylonfb_parse_layer_info(struct device_node *parent_dn,
				    struct xylonfb_data *data, int id)
{
	struct device_node *dn;
	struct xylonfb_layer_fix_data *fd;
	int ret;
	char layer_name[10];
	const char *string;

	XYLONFB_DBG(INFO, "%s", __func__);

	snprintf(layer_name, sizeof(layer_name), "layer_%d", id);
	dn = of_get_child_by_name(parent_dn, layer_name);
	if (!dn)
		return 0;

	data->layers++;

	fd = devm_kzalloc(&data->pdev->dev,
			  sizeof(struct xylonfb_layer_fix_data), GFP_KERNEL);
	if (!fd) {
		pr_err("failed allocate layer fix data (%d)\n", id);
		return -ENOMEM;
	}

	data->fd[id] = fd;

	fd->id = id;

	ret = of_property_read_u32(dn, "address", &fd->address);
	if (ret && (ret != -EINVAL)) {
		pr_err("failed get address\n");
		return ret;
	}
	ret = of_property_read_u32_index(dn, "address", 1, &fd->address_range);

	ret = of_property_read_u32(dn, "buffer-offset", &fd->buffer_offset);
	if (ret && (ret != -EINVAL)) {
		pr_err("failed get buffer-offset\n");
		return ret;
	}

	ret = of_property_read_u32(dn, "bits-per-pixel", &fd->bpp);
	if (ret) {
		pr_err("failed get bits-per-pixel\n");
		return ret;
	}
	switch (fd->bpp) {
	case 8:
	case 16:
	case 32:
		break;
	default:
		pr_err("invalid bits-per-pixel value\n");
		return -EINVAL;
	}

	ret = of_property_read_string(dn, "type", &string);
	if (ret) {
		pr_err("failed get type\n");
		return ret;
	}
	if (!strcmp(string, "alpha")) {
		fd->type = LOGICVC_LAYER_ALPHA;
	} else if (!strcmp(string, "rgb")) {
		fd->type = LOGICVC_LAYER_RGB;
	} else if (!strcmp(string, "yuv")) {
		fd->type = LOGICVC_LAYER_YUV;
	} else {
		pr_err("unsupported layer type\n");
		return -EINVAL;
	}

	if (fd->type != LOGICVC_LAYER_ALPHA) {
		ret = of_property_read_string(dn, "transparency", &string);
		if (ret) {
			pr_err("failed get transparency\n");
			return ret;
		}
		if (!strcmp(string, "clut16")) {
			fd->transparency = LOGICVC_ALPHA_CLUT_16BPP;
		} else if (!strcmp(string, "clut32")) {
			fd->transparency = LOGICVC_ALPHA_CLUT_32BPP;
		} else if (!strcmp(string, "layer")) {
			fd->transparency = LOGICVC_ALPHA_LAYER;
		} else if (!strcmp(string, "pixel")) {
			fd->transparency = LOGICVC_ALPHA_PIXEL;
		} else {
			pr_err("unsupported layer transparency\n");
			return -EINVAL;
		}
	}

	if (of_property_read_bool(dn, "component-swap"))
		fd->component_swap = true;

	fd->width = data->pixel_stride;

	ret = xylonfb_layer_set_format(fd);
	if (ret) {
		pr_err("failed set layer format\n");
		return ret;
	}

	of_node_put(dn);

	return id + 1;
}

static int xylon_parse_hw_info(struct device_node *dn,
			       struct xylonfb_data *data)
{
	int ret;
	const char *string;

	XYLONFB_DBG(INFO, "%s", __func__);

	ret = of_property_read_u32(dn, "background-layer-bits-per-pixel",
				   &data->bg_layer_bpp);
	if (ret && (ret != -EINVAL)) {
		pr_err("failed get bg-layer-bits-per-pixel\n");
		return ret;
	} else if (ret == 0) {
		data->flags |= XYLONFB_FLAGS_BACKGROUND_LAYER;

		ret = of_property_read_string(dn, "background-layer-type",
					      &string);
		if (ret) {
			pr_err("failed get bg-layer-type\n");
			return ret;
		}
		if (!strcmp(string, "rgb")) {
			data->flags |= XYLONFB_FLAGS_BACKGROUND_LAYER_RGB;
		} else if (!strcmp(string, "yuv")) {
			data->flags |= XYLONFB_FLAGS_BACKGROUND_LAYER_YUV;
		} else {
			pr_err("unsupported bg layer type\n");
			return -EINVAL;
		}
	}

	if (of_property_read_bool(dn, "display-interface-itu656"))
		data->flags |= XYLONFB_FLAGS_DISPLAY_INTERFACE_ITU656;

	if (of_property_read_bool(dn, "readable-regs"))
		data->flags |= XYLONFB_FLAGS_READABLE_REGS;
	else
		pr_info("logicvc registers not readable\n");

	if (of_property_read_bool(dn, "size-position"))
		data->flags |= XYLONFB_FLAGS_SIZE_POSITION;
	else
		pr_info("logicvc size-position disabled\n");

	ret = of_property_read_u32(dn, "pixel-stride", &data->pixel_stride);
	if (ret) {
		pr_err("failed get pixel-stride\n");
		return ret;
	}

	ret = of_property_read_u32(dn, "power-delay", &data->pwr_delay);
	if (ret && (ret != -EINVAL)) {
		pr_err("failed get power-delay\n");
		return ret;
	}

	ret = of_property_read_u32(dn, "signal-delay", &data->sig_delay);
	if (ret && (ret != -EINVAL)) {
		pr_err("failed get signal\n");
		return ret;
	}

	return 0;
}

static const struct of_device_id logicvc_of_match[] = {
	{ .compatible = "xylon,logicvc-4.00.a" },
	{ .compatible = "xylon,logicvc-4.01.a" },
	{ .compatible = "xylon,logicvc-4.01.b" },
	{ .compatible = "xylon,logicvc-3.01.a" },
	{ .compatible = "xylon,logicvc-3.02.a" },
	{/* end of table */}
};

static int xylonfb_get_logicvc_configuration(struct xylonfb_data *data)
{
	struct device_node *dn = data->device;
	const struct of_device_id *match;
	int i, ret;

	XYLONFB_DBG(INFO, "%s", __func__);

	match = of_match_node(logicvc_of_match, dn);
	if (!match) {
		pr_err("failed match logicvc\n");
		return -ENODEV;
	}

	ret = of_address_to_resource(dn, 0, &data->resource_mem);
	if (ret) {
		pr_err("failed get mem resource\n");
		return ret;
	}
	data->irq = of_irq_to_resource(dn, 0, &data->resource_irq);
	if (data->irq == 0) {
		pr_err("failed get irq resource\n");
		return ret;
	}

	ret = xylon_parse_hw_info(dn, data);
	if (ret)
		return ret;

	for (i = 0; i < LOGICVC_MAX_LAYERS; i++) {
		ret = xylonfb_parse_layer_info(dn, data, i);
		if (ret < 0)
			return ret;
		if (ret == 0)
			break;
	}

	if (data->flags & XYLONFB_FLAGS_BACKGROUND_LAYER &&
	    data->layers == LOGICVC_MAX_LAYERS) {
		data->flags &= ~XYLONFB_FLAGS_BACKGROUND_LAYER;
		data->layers--;
		if (data->console_layer == data->layers)
			data->console_layer--;

		pr_info("invalid last layer configuration\n");
	}

	xylonfb_init_ctrl(data, dn, &data->vm.ctrl);

	return 0;
}

static int xylonfb_get_driver_configuration(struct xylonfb_data *data)
{
	struct device_node *dn = data->pdev->dev.of_node;
	struct device_node *disp_timings, *timings_dn;
	struct fb_videomode *vm;
	u32 val;
	int ret;
	const char *string;
	char *vm_name;

	XYLONFB_DBG(INFO, "%s", __func__);

	data->device = of_parse_phandle(dn, "device", 0);
	if (!data->device) {
		pr_err("failed get device\n");
		return -ENODEV;
	}

	data->encoder = of_parse_phandle(dn, "encoder", 0);
	if (!data->encoder)
		pr_warn("no available encoder\n");

	data->pixel_clock = of_parse_phandle(dn, "clocks", 0);
	if (!data->pixel_clock)
		pr_warn("no available clocks\n");

	ret = of_property_read_u32(dn, "console-layer", &data->console_layer);
	if (ret && (ret != -EINVAL)) {
			pr_err("failed get console-layer\n");
			return ret;
	} else {
		data->flags |= XYLONFB_FLAGS_CHECK_CONSOLE_LAYER;
	}

	if (of_property_read_bool(dn, "edid-video-mode")) {
		data->flags |= XYLONFB_FLAGS_EDID_VMODE;
		if (of_property_read_bool(dn, "edid-print"))
			data->flags |= XYLONFB_FLAGS_EDID_PRINT;
	} else {
		data->flags |= XYLONFB_FLAGS_ADV7511_SKIP;
	}

	if (of_property_read_bool(dn, "vsync-irq"))
		data->flags |= XYLONFB_FLAGS_VSYNC_IRQ;

	ret = of_property_read_string(dn, "video-mode", &string);
	if (ret && (ret != -EINVAL)) {
		pr_err("failed get video-mode\n");
		return ret;
	} else if (ret == 0) {
		strcpy(data->vm.name, string);
		return 0;
	}

	disp_timings = of_find_node_by_name(dn, "display-timings");
	if (disp_timings) {
		vm = &data->vm.vmode;
		vm_name = data->vm.name;

		if (of_property_read_bool(disp_timings, "native-mode"))
			timings_dn = of_parse_phandle(disp_timings,
						      "native-mode", 0);
		else
			timings_dn = of_get_next_child(disp_timings, NULL);

		strcpy(data->vm.name, timings_dn->name);

		if (timings_dn) {
			if (of_property_read_u32(timings_dn, "clock-frequency",
						 &vm->pixclock)) {
				pr_err("failed get clock-frequency\n");
				goto node_put_param;
			}
			vm->pixclock = KHZ2PICOS(vm->pixclock / 1000);

			if (of_property_read_u32(timings_dn, "hactive",
						 &vm->xres)) {
				pr_err("failed get hactive\n");
				goto node_put_param;
			}

			if (of_property_read_u32(timings_dn, "vactive",
						 &vm->yres)) {
				pr_err("failed get vactive\n");
				goto node_put_param;
			}

			if (of_property_read_u32(timings_dn, "hfront-porch",
						 &vm->right_margin)) {
				pr_err("failed get hfront-porch\n");
				goto node_put_param;
			}

			if (of_property_read_u32(timings_dn, "hback-porch",
						 &vm->left_margin)) {
				pr_err("failed get hback-porch\n");
				goto node_put_param;
			}

			if (of_property_read_u32(timings_dn, "hsync-len",
						 &vm->hsync_len)) {
				pr_err("failed get hsync-len\n");
				goto node_put_param;
			}

			if (of_property_read_u32(timings_dn, "vfront-porch",
						 &vm->lower_margin)) {
				pr_err("failed get vfront-porch\n");
				goto node_put_param;
			}

			if (of_property_read_u32(timings_dn, "vback-porch",
						 &vm->upper_margin)) {
				pr_err("failed get vback-porch\n");
				goto node_put_param;
			}

			if (of_property_read_u32(timings_dn, "vsync-len",
						 &vm->vsync_len)) {
				pr_err("failed get vsync-len\n");
				goto node_put_param;
			}

			ret = of_property_read_u32(timings_dn, "hsync-active",
						 &val);
			if (ret && (ret != -EINVAL)) {
				pr_err("failed get hsync-active\n");
				goto node_put_param;
			} else if ((ret == 0) && (val == 0)) {
				data->hw_flags |= LOGICVC_CTRL_HSYNC_INVERT;
			}

			ret = of_property_read_u32(timings_dn, "vsync-active",
						 &val);
			if (ret && (ret != -EINVAL)) {
				pr_err("failed get vsync-active\n");
				goto node_put_param;
			} else if ((ret == 0) && (val == 0)) {
				data->hw_flags |= LOGICVC_CTRL_VSYNC_INVERT;
			}

			ret = of_property_read_u32(timings_dn, "de-active",
						 &val);
			if (ret && (ret != -EINVAL)) {
				pr_err("failed get de-active\n");
				goto node_put_param;
			} else if ((ret == 0) && (val == 0)) {
				data->hw_flags |=
					LOGICVC_CTRL_PIXEL_DATA_INVERT;
			}

			ret = of_property_read_u32(timings_dn,
						   "pixelclk-active", &val);
			if (ret && (ret != -EINVAL)) {
				pr_err("failed get pixelclk-active\n");
				goto node_put_param;
			} else if ((ret == 0) && (val == 1)) {
				data->hw_flags |=
					LOGICVC_CTRL_PIXEL_DATA_TRIGGER_INVERT;
			}

			data->flags |= XYLONFB_FLAGS_VMODE_CUSTOM;
node_put_param:
			of_node_put(timings_dn);
		}

		of_node_put(disp_timings);
	} else {
		pr_info("default video mode\n");
		return 0;
	}

	return 0;
}

static int xylonfb_probe(struct platform_device *pdev)
{
	struct xylonfb_data *data;
	int ret;

	XYLONFB_DBG(INFO, "%s", __func__);

	data = devm_kzalloc(&pdev->dev, sizeof(struct xylonfb_data),
			     GFP_KERNEL);
	if (!data) {
		pr_err("failed allocate init data\n");
		return -ENOMEM;
	}

	data->pdev = pdev;

	ret = xylonfb_get_driver_configuration(data);
	if (ret)
		goto xylonfb_probe_error;

	ret = xylonfb_get_logicvc_configuration(data);
	if (ret)
		goto xylonfb_probe_error;

	ret = xylonfb_init_core(data);

xylonfb_probe_error:
	return ret;
}

static int xylonfb_remove(struct platform_device *pdev)
{
	XYLONFB_DBG(INFO, "%s", __func__);

	return xylonfb_deinit_core(pdev);
}

static const struct of_device_id xylonfb_of_match[] = {
	{ .compatible = "xylon,fb-3.00.a" },
	{/* end of table */},
};
MODULE_DEVICE_TABLE(of, xylonfb_of_match);

static struct platform_driver xylonfb_driver = {
	.probe = xylonfb_probe,
	.remove = xylonfb_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = XYLONFB_DEVICE_NAME,
		.of_match_table = xylonfb_of_match,
	},
};

#ifndef MODULE
static int xylonfb_get_params(char *options)
{
	char *this_opt;

	XYLONFB_DBG(INFO, "%s", __func__);

	if (!options || !*options)
		return 0;

	while ((this_opt = strsep(&options, ",")) != NULL) {
		if (!*this_opt)
			continue;
		xylonfb_mode_option = this_opt;
	}
	return 0;
}
#endif

static int xylonfb_init(void)
{
#ifndef MODULE
	char *option = NULL;
	/*
	 *  Kernel boot options (in 'video=xxxfb:<options>' format)
	 */
	if (fb_get_options(XYLONFB_DRIVER_NAME, &option))
		return -ENODEV;
	/* Set internal module parameters */
	xylonfb_get_params(option);
#endif
	if (platform_driver_register(&xylonfb_driver)) {
		pr_err("Error driver registration\n");
		return -ENODEV;
	}

	return 0;
}

static void __exit xylonfb_exit(void)
{
	platform_driver_unregister(&xylonfb_driver);
}

#ifndef MODULE
late_initcall(xylonfb_init);
#else
module_init(xylonfb_init);
module_exit(xylonfb_exit);
#endif

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION(XYLONFB_DRIVER_DESCRIPTION);
MODULE_VERSION(XYLONFB_DRIVER_VERSION);
