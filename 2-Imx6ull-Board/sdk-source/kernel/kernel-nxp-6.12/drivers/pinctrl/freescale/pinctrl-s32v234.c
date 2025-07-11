// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * s32v234 pinctrl driver based on imx pinmux and pinconf core
 *
 * Copyright 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2017, 2019 NXP
 */

#include <linux/err.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/platform_device.h>
#include <dt-bindings/pinctrl/s32v234-pinctrl.h>

#include "pinctrl-s32v.h"

/* Pad names for the pinmux subsystem */
static const struct pinctrl_pin_desc s32v234_pinctrl_pads[] = {
	S32V_PINCTRL_PIN(S32V234_MSCR_PA0),
	S32V_PINCTRL_PIN(S32V234_MSCR_PA1),
	S32V_PINCTRL_PIN(S32V234_MSCR_PA2),
	S32V_PINCTRL_PIN(S32V234_MSCR_PA3),
	S32V_PINCTRL_PIN(S32V234_MSCR_PA4),
	S32V_PINCTRL_PIN(S32V234_MSCR_PA5),
	S32V_PINCTRL_PIN(S32V234_MSCR_PA6),
	S32V_PINCTRL_PIN(S32V234_MSCR_PA7),
	S32V_PINCTRL_PIN(S32V234_MSCR_PA8),
	S32V_PINCTRL_PIN(S32V234_MSCR_PA9),
	S32V_PINCTRL_PIN(S32V234_MSCR_PA10),
	S32V_PINCTRL_PIN(S32V234_MSCR_PA11),
	S32V_PINCTRL_PIN(S32V234_MSCR_PA12),
	S32V_PINCTRL_PIN(S32V234_MSCR_PA13),
	S32V_PINCTRL_PIN(S32V234_MSCR_PA14),
	S32V_PINCTRL_PIN(S32V234_MSCR_PA15),
	S32V_PINCTRL_PIN(S32V234_MSCR_PB0),
	S32V_PINCTRL_PIN(S32V234_MSCR_PB1),
	S32V_PINCTRL_PIN(S32V234_MSCR_PB2),
	S32V_PINCTRL_PIN(S32V234_MSCR_PB3),
	S32V_PINCTRL_PIN(S32V234_MSCR_PB4),
	S32V_PINCTRL_PIN(S32V234_MSCR_PB5),
	S32V_PINCTRL_PIN(S32V234_MSCR_PB6),
	S32V_PINCTRL_PIN(S32V234_MSCR_PB7),
	S32V_PINCTRL_PIN(S32V234_MSCR_PB8),
	S32V_PINCTRL_PIN(S32V234_MSCR_PB9),
	S32V_PINCTRL_PIN(S32V234_MSCR_PB10),
	S32V_PINCTRL_PIN(S32V234_MSCR_PB11),
	S32V_PINCTRL_PIN(S32V234_MSCR_PB12),
	S32V_PINCTRL_PIN(S32V234_MSCR_PB13),
	S32V_PINCTRL_PIN(S32V234_MSCR_PB14),
	S32V_PINCTRL_PIN(S32V234_MSCR_PB15),
	S32V_PINCTRL_PIN(S32V234_MSCR_PC0),
	S32V_PINCTRL_PIN(S32V234_MSCR_PC1),
	S32V_PINCTRL_PIN(S32V234_MSCR_PC2),
	S32V_PINCTRL_PIN(S32V234_MSCR_PC3),
	S32V_PINCTRL_PIN(S32V234_MSCR_PC4),
	S32V_PINCTRL_PIN(S32V234_MSCR_PC5),
	S32V_PINCTRL_PIN(S32V234_MSCR_PC6),
	S32V_PINCTRL_PIN(S32V234_MSCR_PC7),
	S32V_PINCTRL_PIN(S32V234_MSCR_PC8),
	S32V_PINCTRL_PIN(S32V234_MSCR_PC9),
	S32V_PINCTRL_PIN(S32V234_MSCR_PC10),
	S32V_PINCTRL_PIN(S32V234_MSCR_PC11),
	S32V_PINCTRL_PIN(S32V234_MSCR_PC12),
	S32V_PINCTRL_PIN(S32V234_MSCR_PC13),
	S32V_PINCTRL_PIN(S32V234_MSCR_PC14),
	S32V_PINCTRL_PIN(S32V234_MSCR_PC15),
	S32V_PINCTRL_PIN(S32V234_MSCR_PD0),
	S32V_PINCTRL_PIN(S32V234_MSCR_PD1),
	S32V_PINCTRL_PIN(S32V234_MSCR_PD2),
	S32V_PINCTRL_PIN(S32V234_MSCR_PD3),
	S32V_PINCTRL_PIN(S32V234_MSCR_PD4),
	S32V_PINCTRL_PIN(S32V234_MSCR_PD5),
	S32V_PINCTRL_PIN(S32V234_MSCR_PD6),
	S32V_PINCTRL_PIN(S32V234_MSCR_PD7),
	S32V_PINCTRL_PIN(S32V234_MSCR_PD8),
	S32V_PINCTRL_PIN(S32V234_MSCR_PD9),
	S32V_PINCTRL_PIN(S32V234_MSCR_PD10),
	S32V_PINCTRL_PIN(S32V234_MSCR_PD11),
	S32V_PINCTRL_PIN(S32V234_MSCR_PD12),
	S32V_PINCTRL_PIN(S32V234_MSCR_PD13),
	S32V_PINCTRL_PIN(S32V234_MSCR_PD14),
	S32V_PINCTRL_PIN(S32V234_MSCR_PD15),
	S32V_PINCTRL_PIN(S32V234_MSCR_PE0),
	S32V_PINCTRL_PIN(S32V234_MSCR_PE1),
	S32V_PINCTRL_PIN(S32V234_MSCR_PE2),
	S32V_PINCTRL_PIN(S32V234_MSCR_PE3),
	S32V_PINCTRL_PIN(S32V234_MSCR_PE4),
	S32V_PINCTRL_PIN(S32V234_MSCR_PE5),
	S32V_PINCTRL_PIN(S32V234_MSCR_PE6),
	S32V_PINCTRL_PIN(S32V234_MSCR_PE7),
	S32V_PINCTRL_PIN(S32V234_MSCR_PE8),
	S32V_PINCTRL_PIN(S32V234_MSCR_PE9),
	S32V_PINCTRL_PIN(S32V234_MSCR_PE10),
	S32V_PINCTRL_PIN(S32V234_MSCR_PE11),
	S32V_PINCTRL_PIN(S32V234_MSCR_PE12),
	S32V_PINCTRL_PIN(S32V234_MSCR_PE13),
	S32V_PINCTRL_PIN(S32V234_MSCR_PE14),
	S32V_PINCTRL_PIN(S32V234_MSCR_PE15),
	S32V_PINCTRL_PIN(S32V234_MSCR_PF0),
	S32V_PINCTRL_PIN(S32V234_MSCR_PF1),
	S32V_PINCTRL_PIN(S32V234_MSCR_PF2),
	S32V_PINCTRL_PIN(S32V234_MSCR_PF3),
	S32V_PINCTRL_PIN(S32V234_MSCR_PF4),
	S32V_PINCTRL_PIN(S32V234_MSCR_PF5),
	S32V_PINCTRL_PIN(S32V234_MSCR_PF6),
	S32V_PINCTRL_PIN(S32V234_MSCR_PF7),
	S32V_PINCTRL_PIN(S32V234_MSCR_PF8),
	S32V_PINCTRL_PIN(S32V234_MSCR_PF9),
	S32V_PINCTRL_PIN(S32V234_MSCR_PF10),
	S32V_PINCTRL_PIN(S32V234_MSCR_PF11),
	S32V_PINCTRL_PIN(S32V234_MSCR_PF12),
	S32V_PINCTRL_PIN(S32V234_MSCR_PF13),
	S32V_PINCTRL_PIN(S32V234_MSCR_PF14),
	S32V_PINCTRL_PIN(S32V234_MSCR_PF15),
	S32V_PINCTRL_PIN(S32V234_MSCR_PG0),
	S32V_PINCTRL_PIN(S32V234_MSCR_PG1),
	S32V_PINCTRL_PIN(S32V234_MSCR_PG2),
	S32V_PINCTRL_PIN(S32V234_MSCR_PG3),
	S32V_PINCTRL_PIN(S32V234_MSCR_PG4),
	S32V_PINCTRL_PIN(S32V234_MSCR_PG5),
	S32V_PINCTRL_PIN(S32V234_MSCR_PG6),
	S32V_PINCTRL_PIN(S32V234_MSCR_PG7),
	S32V_PINCTRL_PIN(S32V234_MSCR_PG8),
	S32V_PINCTRL_PIN(S32V234_MSCR_PG9),
	S32V_PINCTRL_PIN(S32V234_MSCR_PG10),
	S32V_PINCTRL_PIN(S32V234_MSCR_PG11),
	S32V_PINCTRL_PIN(S32V234_MSCR_PG12),
	S32V_PINCTRL_PIN(S32V234_MSCR_PG13),
	S32V_PINCTRL_PIN(S32V234_MSCR_PG14),
	S32V_PINCTRL_PIN(S32V234_MSCR_PG15),
	S32V_PINCTRL_PIN(S32V234_MSCR_PH0),
	S32V_PINCTRL_PIN(S32V234_MSCR_PH1),
	S32V_PINCTRL_PIN(S32V234_MSCR_PH2),
	S32V_PINCTRL_PIN(S32V234_MSCR_PH3),
	S32V_PINCTRL_PIN(S32V234_MSCR_PH4),
	S32V_PINCTRL_PIN(S32V234_MSCR_PH5),
	S32V_PINCTRL_PIN(S32V234_MSCR_PH6),
	S32V_PINCTRL_PIN(S32V234_MSCR_PH7),
	S32V_PINCTRL_PIN(S32V234_MSCR_PH8),
	S32V_PINCTRL_PIN(S32V234_MSCR_PH9),
	S32V_PINCTRL_PIN(S32V234_MSCR_PH10),
	S32V_PINCTRL_PIN(S32V234_MSCR_PH11),
	S32V_PINCTRL_PIN(S32V234_MSCR_PH12),
	S32V_PINCTRL_PIN(S32V234_MSCR_PH13),
	S32V_PINCTRL_PIN(S32V234_MSCR_PH14),
	S32V_PINCTRL_PIN(S32V234_MSCR_PH15),
	S32V_PINCTRL_PIN(S32V234_MSCR_PJ0),
	S32V_PINCTRL_PIN(S32V234_MSCR_PJ1),
	S32V_PINCTRL_PIN(S32V234_MSCR_PJ2),
	S32V_PINCTRL_PIN(S32V234_MSCR_PJ3),
	S32V_PINCTRL_PIN(S32V234_MSCR_PJ4),
	S32V_PINCTRL_PIN(S32V234_MSCR_PJ5),
	S32V_PINCTRL_PIN(S32V234_MSCR_PJ6),
	S32V_PINCTRL_PIN(S32V234_MSCR_PJ7),
	S32V_PINCTRL_PIN(S32V234_MSCR_PJ8),
	S32V_PINCTRL_PIN(S32V234_MSCR_PJ9),
	S32V_PINCTRL_PIN(S32V234_MSCR_PJ10),
	S32V_PINCTRL_PIN(S32V234_MSCR_PJ11),
	S32V_PINCTRL_PIN(S32V234_MSCR_PJ12),
	S32V_PINCTRL_PIN(S32V234_MSCR_PJ13),
	S32V_PINCTRL_PIN(S32V234_MSCR_PJ14),
	S32V_PINCTRL_PIN(S32V234_MSCR_PJ15),
	S32V_PINCTRL_PIN(S32V234_MSCR_PK0),
	S32V_PINCTRL_PIN(S32V234_MSCR_PK1),
	S32V_PINCTRL_PIN(S32V234_MSCR_PK2),
	S32V_PINCTRL_PIN(S32V234_MSCR_PK3),
	S32V_PINCTRL_PIN(S32V234_MSCR_PK4),
	S32V_PINCTRL_PIN(S32V234_MSCR_PK5),
	S32V_PINCTRL_PIN(S32V234_MSCR_PK6),
	S32V_PINCTRL_PIN(S32V234_MSCR_PK7),
	S32V_PINCTRL_PIN(S32V234_MSCR_PK8),
	S32V_PINCTRL_PIN(S32V234_MSCR_PK9),
	S32V_PINCTRL_PIN(S32V234_MSCR_PK10),
	S32V_PINCTRL_PIN(S32V234_MSCR_PK11),
	S32V_PINCTRL_PIN(S32V234_MSCR_PK12),
	S32V_PINCTRL_PIN(S32V234_MSCR_PK13),
	S32V_PINCTRL_PIN(S32V234_MSCR_PK14),
	S32V_PINCTRL_PIN(S32V234_MSCR_PK15),
	S32V_PINCTRL_PIN(S32V234_MSCR_PL0),
	S32V_PINCTRL_PIN(S32V234_MSCR_PL1),
	S32V_PINCTRL_PIN(S32V234_MSCR_PL2),
	S32V_PINCTRL_PIN(S32V234_MSCR_PL3),
	S32V_PINCTRL_PIN(S32V234_MSCR_PL4),
	S32V_PINCTRL_PIN(S32V234_MSCR_PL5),
	S32V_PINCTRL_PIN(S32V234_MSCR_PL8),

	S32V_PINCTRL_PIN(S32V234_IMCR_USDHC_CLK),
	S32V_PINCTRL_PIN(S32V234_IMCR_USDHC_CMD),
	S32V_PINCTRL_PIN(S32V234_IMCR_USDHC_DAT0),
	S32V_PINCTRL_PIN(S32V234_IMCR_USDHC_DAT1),
	S32V_PINCTRL_PIN(S32V234_IMCR_USDHC_DAT2),
	S32V_PINCTRL_PIN(S32V234_IMCR_USDHC_DAT3),
	S32V_PINCTRL_PIN(S32V234_IMCR_USDHC_DAT4),
	S32V_PINCTRL_PIN(S32V234_IMCR_USDHC_DAT5),
	S32V_PINCTRL_PIN(S32V234_IMCR_USDHC_DAT6),
	S32V_PINCTRL_PIN(S32V234_IMCR_USDHC_DAT7),
	S32V_PINCTRL_PIN(S32V234_IMCR_CAN_FD0_RXD),
	S32V_PINCTRL_PIN(S32V234_IMCR_CAN_FD1_RXD),
	S32V_PINCTRL_PIN(S32V234_IMCR_UART0_RXD),
	S32V_PINCTRL_PIN(S32V234_IMCR_UART1_RXD),
	S32V_PINCTRL_PIN(S32V234_IMCR_USDHC_WP),

	S32V_PINCTRL_PIN(S32V234_IMCR_Ethernet_RX_ER),
	S32V_PINCTRL_PIN(S32V234_IMCR_Ethernet_COL),
	S32V_PINCTRL_PIN(S32V234_IMCR_Ethernet_CRS),
	S32V_PINCTRL_PIN(S32V234_IMCR_Ethernet_RX_DV),
	S32V_PINCTRL_PIN(S32V234_IMCR_Ethernet_RX_D0),
	S32V_PINCTRL_PIN(S32V234_IMCR_Ethernet_RX_D1),
	S32V_PINCTRL_PIN(S32V234_IMCR_Ethernet_RX_D2),
	S32V_PINCTRL_PIN(S32V234_IMCR_Ethernet_RX_D3),
	S32V_PINCTRL_PIN(S32V234_IMCR_Ethernet_TX_CLK),
	S32V_PINCTRL_PIN(S32V234_IMCR_Ethernet_RX_CLK),
	S32V_PINCTRL_PIN(S32V234_IMCR_Ethernet_MDIO),
	S32V_PINCTRL_PIN(S32V234_IMCR_Ethernet_TIMER0),
	S32V_PINCTRL_PIN(S32V234_IMCR_Ethernet_TIMER1),
	S32V_PINCTRL_PIN(S32V234_IMCR_Ethernet_TIMER2),
};

static struct s32v_pinctrl_soc_info s32v234_pinctrl_info = {
	.pins = s32v234_pinctrl_pads,
	.npins = ARRAY_SIZE(s32v234_pinctrl_pads),
};

static const struct of_device_id s32v234_pinctrl_of_match[] = {
	{ .compatible = "fsl,s32v234-siul2", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, s32v234_pinctrl_of_match);

static int s32v234_pinctrl_probe(struct platform_device *pdev)
{
	return s32v_pinctrl_probe(pdev, &s32v234_pinctrl_info);
}

static struct platform_driver s32v234_pinctrl_driver = {
	.driver = {
		.name = "s32v234-siul2",
		.owner = THIS_MODULE,
		.of_match_table = s32v234_pinctrl_of_match,
	},
	.probe = s32v234_pinctrl_probe,
	.remove = s32v_pinctrl_remove,
};

module_platform_driver(s32v234_pinctrl_driver);

MODULE_DESCRIPTION("Freescale S32V234 pinctrl driver");
MODULE_LICENSE("GPL v2");
