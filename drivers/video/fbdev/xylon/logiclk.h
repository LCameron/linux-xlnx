/*
 * Xylon pixel clock generation logiCLK IP core interface
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

#include <linux/types.h>

#define LOGICLK_REGS			21
#define LOGICLK_OUTPUTS			6
#define LOGICLK_RST_REG_OFF		0
#define LOGICLK_PLL_REG_OFF		1
#define LOGICLK_PLL_MANUAL_REG_OFF	3
#define LOGICLK_PLL_RDY			0x01
#define LOGICLK_PLL_EN			0x01
#define LOGICLK_PLL_REG_EN		0x02

struct logiclk_freq_out {
	u32 freq_out_hz[LOGICLK_OUTPUTS];
};

/*
	Calculates the output register valuess depending on the
	"freq_out" and "c_osc_clk_freq_hz" inputs.
	Writes them to array of LOGICLK_REGS over "regs_out" pointer.
*/
int logiclk_calc_regs(struct logiclk_freq_out *freq_out,
		      u32 c_osc_clk_freq_hz, u32 *regs_out);
