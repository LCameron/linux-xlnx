/*
 * Xylon logiCVC frame buffer driver pixel clock generation
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

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/of.h>

#if defined(CONFIG_FB_XYLON_PIXCLK_LOGICLK)

#include <linux/delay.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/of_address.h>

#include "logiclk.h"

bool xylonfb_hw_pixclk_supported(struct device *dev, struct device_node *dn);
void xylonfb_hw_pixclk_unload(void);
int xylonfb_hw_pixclk_set(struct device_node *dn, unsigned long pixclk_khz);

struct logiclk {
	struct device_node *dn;
	struct resource res;
	void __iomem *base;
	u32 osc_freq_hz;
};
static struct logiclk logiclk_dev;

static const struct of_device_id logiclk_of_match[] = {
	{ .compatible = "xylon,logiclk-1.02.b" },
	{/* end of table */}
};

static int xylonfb_hw_pixclk_set_logiclk(struct device_node *dn,
					 unsigned long pixclk_khz)
{
	struct logiclk_freq_out freq_out;
	u32 regs[LOGICLK_REGS];
	int i;

pr_info("%s\n", __func__);

	if (dn != logiclk_dev.dn)
		return -ENODEV;

	for (i = 0; i < LOGICLK_OUTPUTS; i++)
		freq_out.freq_out_hz[i] = pixclk_khz * 1000;

	if (logiclk_calc_regs(&freq_out, logiclk_dev.osc_freq_hz, regs)) {
		pr_err("failed calculate logiclk parameters\n");
		return -EINVAL;
	}

	writel(1, logiclk_dev.base + LOGICLK_RST_REG_OFF);
	udelay(10);
	writel(0, logiclk_dev.base + LOGICLK_RST_REG_OFF);

	for (i = 0; i < LOGICLK_REGS; i++)
		writel(regs[i], logiclk_dev.base +
		       (LOGICLK_PLL_MANUAL_REG_OFF + i));

	while (1) {
		if (readl(logiclk_dev.base + LOGICLK_PLL_REG_OFF) &
			  LOGICLK_PLL_RDY) {
			writel((LOGICLK_PLL_REG_EN | LOGICLK_PLL_EN),
				logiclk_dev.base + LOGICLK_PLL_REG_OFF);
			break;
		}
	}

	return 0;
}

#endif

#if defined(CONFIG_FB_XYLON_PIXCLK_SI570)

#include <linux/clk.h>

struct si570 {
	struct device_node *dn;
	struct clk *clk;
};
static struct si570 si570_dev;

static int xylonfb_hw_pixclk_set_si570(struct device_node *dn,
				       unsigned long pixclk_khz)
{
pr_info("%s\n", __func__);

	if (dn != si570_dev.dn)
		return -ENODEV;

	if (clk_set_rate(si570_dev.clk, (pixclk_khz * 1000))) {
		pr_err("failed set si570 pixel frequency\n");
		return -EINVAL;
	}

	return 0;
}

#endif

bool xylonfb_hw_pixclk_supported(struct device *dev, struct device_node *dn)
{
#if defined(CONFIG_FB_XYLON_PIXCLK_LOGICLK) || \
    defined(CONFIG_FB_XYLON_PIXCLK_SI570)
	int ret;
#endif

#if defined(CONFIG_FB_XYLON_PIXCLK_LOGICLK)
	const struct of_device_id *match;

	match = of_match_node(logiclk_of_match, dn);
	if (match) {
		ret = of_address_to_resource(dn, 0, &logiclk_dev.res);
		if (ret) {
			pr_err("failed get mem resource\n");
			return false;
		}

		ret = of_property_read_u32(dn, "osc-freq-hz",
					   &logiclk_dev.osc_freq_hz);
		if (ret) {
			pr_err("failed get osc-freq-hz\n");
			return false;
		}

		logiclk_dev.base = devm_ioremap_resource(dev, &logiclk_dev.res);
		if (IS_ERR(logiclk_dev.base)) {
			pr_err("failed ioremap logiclk mem resource\n");
			return false;
		}

		logiclk_dev.dn = dn;

		return true;
	}
#endif

#if defined(CONFIG_FB_XYLON_PIXCLK_SI570)
	ret = of_device_is_compatible(dn, "silabs,si570");
	if (ret != 0) {
		si570_dev.clk = devm_clk_get(dev, NULL);
		if (IS_ERR(si570_dev.clk)) {
			pr_err("failed get si570 pixel clock\n");
			return false;
		}
		if (clk_prepare_enable(si570_dev.clk)) {
			pr_err("failed prepare/enable si570 pixel clock\n");
			return false;
		}

		si570_dev.dn = dn;

		return true;
	}
#endif

	return false;
}

void xylonfb_hw_pixclk_unload(void)
{
pr_info("%s\n", __func__);

#if defined(CONFIG_FB_XYLON_PIXCLK_LOGICLK)
#endif
#if defined(CONFIG_FB_XYLON_PIXCLK_SI570)
	clk_disable_unprepare(si570_dev.clk);
#endif
}

int xylonfb_hw_pixclk_set(struct device_node *dn, unsigned long pixclk_khz)
{
pr_info("%s\n", __func__);

#if !defined(CONFIG_FB_XYLON_PIXCLK)
	pr_info("pixel clock control not supported\n");
#else
#if defined(CONFIG_FB_XYLON_PIXCLK_LOGICLK)
	if (dn == logiclk_dev.dn) {
		xylonfb_hw_pixclk_set_logiclk(dn, pixclk_khz);
		return 0;
	}
#endif
#if defined(CONFIG_FB_XYLON_PIXCLK_SI570)
	if (dn == si570_dev.dn) {
		xylonfb_hw_pixclk_set_si570(dn, pixclk_khz);
		return 0;
	}
#endif
#endif
	return -ENODEV;
}
