/*
 * Xylon logiCVC frame buffer driver IOCTL parameters
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

#ifndef __XYLONFB_H__
#define __XYLONFB_H__

#include <linux/types.h>

struct xylonfb_layer_color {
	__u32 raw_rgb;
	__u8 use_raw;
	__u8 r;
	__u8 g;
	__u8 b;
	bool set;
};

struct xylonfb_layer_geometry {
	__u16 x;
	__u16 y;
	__u16 width;
	__u16 height;
	bool set;
};

struct xylonfb_layer_transparency {
	__u8 alpha;
	bool set;
};

struct xylonfb_hw_access {
	__u32 offset;
	__u32 value;
	bool set;
};

/* Xylon FB events */
#define XYLONFB_EVENT_FBI_UPDATE (1 << 0)

/* Xylon FB IOCTL's */
#define XYLONFB_IOW(num, dtype)		_IOW('x', num, dtype)
#define XYLONFB_IOR(num, dtype)		_IOR('x', num, dtype)
#define XYLONFB_IOWR(num, dtype)	_IOWR('x', num, dtype)
#define XYLONFB_IO(num)			_IO('x', num)

#define XYLONFB_VSYNC_CTRL		XYLONFB_IOR(30, unsigned char)
#define XYLONFB_LAYER_IDX		XYLONFB_IOR(31, unsigned int)
#define XYLONFB_LAYER_ALPHA		\
	XYLONFB_IOR(32, struct xylonfb_layer_transparency)
#define XYLONFB_LAYER_COLOR_TRANSP_CTRL	XYLONFB_IOW(33, unsigned char)
#define XYLONFB_LAYER_COLOR_TRANSP \
	XYLONFB_IOR(34, struct xylonfb_layer_color)
#define XYLONFB_LAYER_GEOMETRY \
	XYLONFB_IOR(35, struct xylonfb_layer_geometry)
#define XYLONFB_LAYER_BUFFER_OFFSET	XYLONFB_IOR(36, unsigned int)
#define XYLONFB_LAYER_BUFFER_SWITCH	XYLONFB_IOR(42, unsigned int)
#define XYLONFB_BACKGROUND_COLOR \
	XYLONFB_IOR(37, struct xylonfb_layer_color)
#define XYLONFB_LAYER_EXT_BUFF_SWITCH	XYLONFB_IOW(38, unsigned char)
#define XYLONFB_HW_ACCESS \
	XYLONFB_IOR(39, struct xylonfb_hw_access)
#define XYLONFB_WAIT_EDID		XYLONFB_IOW(40, unsigned int)
#define XYLONFB_GET_EDID		XYLONFB_IOR(41, char)

#endif /* __XYLONFB_H__ */
