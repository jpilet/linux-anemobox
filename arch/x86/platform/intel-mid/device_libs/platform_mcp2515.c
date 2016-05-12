/*
 * platform_mcp2515.c: mcp2515/anemobox platform data initialization file
 *
 * (C) Copyright 2015 Anemomind SaRL
 * Author: Julien Pilet <julien@anemomind.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License.
 */

#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/lnw_gpio.h>
#include <linux/spi/intel_mid_ssp_spi.h>
#include <linux/can/platform/mcp251x.h>
#include <asm/intel-mid.h>
#include "platform_mcp2515.h"

static int mcp2515_board_specific_setup(struct spi_device *spi);
static void tng_ssp_spi_cs_control(u32 command);
static void tng_ssp_spi_platform_pinmux(void);

static int tng_ssp_spi2_CS0_gpio = 110;

static struct mcp251x_platform_data mcp251x_info = {
        .oscillator_frequency = 16 * 1000 * 1000,
        .board_specific_setup = mcp2515_board_specific_setup,
        .power_enable = NULL, // mcp251x_power_enable,
        .transceiver_enable = NULL,
};

static struct intel_mid_ssp_spi_chip chip = {
	.burst_size = DFLT_FIFO_BURST_SIZE,
	.timeout = DFLT_TIMEOUT_VAL,
	/* SPI DMA is current usable on Tangier */
	.dma_enabled = true,
	.cs_control = tng_ssp_spi_cs_control,
	.platform_pinmux = tng_ssp_spi_platform_pinmux,
};

static void tng_ssp_spi_cs_control(u32 command)
{
        //pr_debug("mcp251x: setting CS0 line to %d\n", (command != 0) ? 1 : 0);
	gpio_set_value(tng_ssp_spi2_CS0_gpio, (command == CS_ASSERT) ? 0 : 1);
}

static void tng_ssp_spi_platform_pinmux(void)
{
	int err;
	int saved_muxing;
	/* Request Chip Select gpios */
        pr_debug("mcp251x: configuring CS0 GPIO.\n");
	saved_muxing = gpio_get_alt(tng_ssp_spi2_CS0_gpio);

	lnw_gpio_set_alt(tng_ssp_spi2_CS0_gpio, LNW_GPIO);
	err = gpio_request_one(tng_ssp_spi2_CS0_gpio,
			GPIOF_DIR_OUT|GPIOF_INIT_HIGH, "MCP2515 CS");
	if (err) {
		pr_err("%s: unable to get Chip Select GPIO,\
				fallback to legacy CS mode \n", __func__);
		lnw_gpio_set_alt(tng_ssp_spi2_CS0_gpio, saved_muxing);
		chip.cs_control = NULL;
		chip.platform_pinmux = NULL;
	}
}

static int mcp2515_board_specific_setup(struct spi_device *spi) {
	int err;
	int saved_muxing;

        dev_dbg(&spi->dev, "%s IRQ for GPIO 48: %d\n", __func__, gpio_to_irq(48));

        // Configure CS GPIO
	saved_muxing = gpio_get_alt(tng_ssp_spi2_CS0_gpio);

	lnw_gpio_set_alt(tng_ssp_spi2_CS0_gpio, LNW_GPIO);
	err = devm_gpio_request_one(&spi->dev, tng_ssp_spi2_CS0_gpio,
			GPIOF_DIR_OUT|GPIOF_INIT_HIGH, "MCP2515 CS");
	if (err) {
		pr_err("%s: unable to get Chip Select GPIO,\
				fallback to legacy CS mode \n", __func__);
		lnw_gpio_set_alt(tng_ssp_spi2_CS0_gpio, saved_muxing);
		chip.cs_control = NULL;
		chip.platform_pinmux = NULL;
	}

        // The MCP2515 CAN_INT on the anemobox is connected to GP48.
        spi->irq = gpio_to_irq(48);
        return 0;
}

void __init *mcp2515_platform_data(void *info)
{
	struct spi_board_info *spi_info = info;

	spi_info->mode = SPI_MODE_0;

        // Due to an electric problem of the SPI bus of the anemobox,
        // we can't set the speed higher than 1MHz.
        // In theory, the mcp251x supports 10MHz. However, the edison SPI driver,
        // when asked to output at 10MHz, outputs at 12.5MHz.
	spi_info->max_speed_hz = 10000000 / 10;
	spi_info->controller_data = &chip;
	spi_info->bus_num = FORCE_SPI_BUS_NUM;

	return &mcp251x_info;
}
