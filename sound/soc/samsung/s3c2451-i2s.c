/* sound/soc/samsung/s3c2451-i2s.c
 *
 * ALSA Soc Audio Layer - S3C2451 I2S driver
 *
 * Copyright (c) 2012 FriendlyARM (www.arm9.net)
 *
 * Copyright (c) 2006 Wolfson Microelectronics PLC.
 *	Graeme Gregory graeme.gregory@wolfsonmicro.com
 *	linux@wolfsonmicro.com
 *
 * Copyright (c) 2007, 2004-2005 Simtec Electronics
 *	http://armlinux.simtec.co.uk/
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/module.h>

#include <sound/soc.h>
#include <sound/pcm_params.h>

#include <mach/dma.h>

#include "dma.h"
#include "regs-i2s-v2.h"
#include "s3c-i2s-v2.h"

static struct s3c2410_dma_client s3c2451_dma_client_out = {
	.name		= "I2S PCM Stereo out"
};

static struct s3c2410_dma_client s3c2451_dma_client_in = {
	.name		= "I2S PCM Stereo in"
};

static struct s3c_dma_params s3c2451_i2s_pcm_stereo_out = {
	.client		= &s3c2451_dma_client_out,
	.channel	= DMACH_I2S_OUT,
	.dma_addr	= S3C2410_PA_IIS + S3C2412_IISTXD,
	.dma_size	= 4,
};

static struct s3c_dma_params s3c2451_i2s_pcm_stereo_in = {
	.client		= &s3c2451_dma_client_in,
	.channel	= DMACH_I2S_IN,
	.dma_addr	= S3C2410_PA_IIS + S3C2412_IISRXD,
	.dma_size	= 4,
};

static struct s3c_i2sv2_info s3c2451_i2s;

static int s3c2451_i2s_probe(struct snd_soc_dai *dai)
{
	u32 iismod;
	int ret;

	pr_debug("Entered %s\n", __func__);

	ret = s3c_i2sv2_probe(dai, &s3c2451_i2s, S3C2410_PA_IIS);
	if (ret)
		return ret;

	s3c2451_i2s.dma_capture = &s3c2451_i2s_pcm_stereo_in;
	s3c2451_i2s.dma_playback = &s3c2451_i2s_pcm_stereo_out;

	s3c2451_i2s.iis_cclk = clk_get(dai->dev, "i2s-if");
	if (IS_ERR(s3c2451_i2s.iis_cclk)) {
		pr_err("failed to get i2s-if clock\n");
		iounmap(s3c2451_i2s.regs);
		return PTR_ERR(s3c2451_i2s.iis_cclk);
	}

	/* Set epllref (or i2s-eplldiv) as the source for IIS CLK */
	clk_set_parent(s3c2451_i2s.iis_cclk, clk_get(NULL, "epllref"));

	clk_enable(s3c2451_i2s.iis_cclk);
	pr_info("iis cclk rate %ld Hz\n", clk_get_rate(s3c2451_i2s.iis_cclk));

	iismod = readl(s3c2451_i2s.regs + S3C2412_IISMOD);
	iismod &= ~(1 << 12);
	iismod |= S3C2412_IISMOD_IMS_SYSMUX;
	writel(iismod, s3c2451_i2s.regs + S3C2412_IISMOD);

	/* Configure the I2S pins (GPE0...GPE4) in correct mode */
	s3c_gpio_cfgall_range(S3C2410_GPE(0), 5, S3C_GPIO_SFN(2),
			S3C_GPIO_PULL_NONE);

	return 0;
}

static int s3c2451_i2s_remove(struct snd_soc_dai *dai)
{
	clk_disable(s3c2451_i2s.iis_cclk);
	clk_put(s3c2451_i2s.iis_cclk);
	iounmap(s3c2451_i2s.regs);

	return 0;
}

#define S3C2451_I2S_RATES \
	(SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 | SNDRV_PCM_RATE_16000 | \
	SNDRV_PCM_RATE_22050 | SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 | \
	SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_88200 | SNDRV_PCM_RATE_96000)

#define S3C2451_I2S_FORMATS \
	(SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE | \
	 SNDRV_PCM_FMTBIT_S24_LE)

static const struct snd_soc_dai_ops s3c2451_i2s_dai_ops = {
	.hw_params	= NULL,
};

static struct snd_soc_dai_driver s3c2451_i2s_dai = {
	.probe		= s3c2451_i2s_probe,
	.remove	= s3c2451_i2s_remove,
	.playback = {
		.channels_min	= 2,
		.channels_max	= 2,
		.rates		= S3C2451_I2S_RATES,
		.formats	= S3C2451_I2S_FORMATS,
	},
	.capture = {
		.channels_min	= 2,
		.channels_max	= 2,
		.rates		= S3C2451_I2S_RATES,
		.formats	= S3C2451_I2S_FORMATS,
	},
	.ops = &s3c2451_i2s_dai_ops,
};

static __devinit int s3c2451_iis_dev_probe(struct platform_device *pdev)
{
	return s3c_i2sv2_register_dai(&pdev->dev, -1, &s3c2451_i2s_dai);
}

static __devexit int s3c2451_iis_dev_remove(struct platform_device *pdev)
{
	snd_soc_unregister_dai(&pdev->dev);
	return 0;
}

static struct platform_driver s3c2451_iis_driver = {
	.probe	= s3c2451_iis_dev_probe,
	.remove	= __devexit_p(s3c2451_iis_dev_remove),
	.driver	= {
		.name	= "s3c24xx-iis",
		.owner	= THIS_MODULE,
	},
};

module_platform_driver(s3c2451_iis_driver);

/* Module information */
MODULE_AUTHOR("Ben Dooks, <ben@simtec.co.uk>");
MODULE_DESCRIPTION("S3C2451 I2S SoC Interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:s3c2451-iis");
