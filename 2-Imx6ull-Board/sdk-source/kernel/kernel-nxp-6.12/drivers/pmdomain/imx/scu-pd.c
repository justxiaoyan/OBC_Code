// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 * Copyright 2017-2018 NXP
 *	Dong Aisheng <aisheng.dong@nxp.com>
 *
 * Implementation of the SCU based Power Domains
 *
 * NOTE: a better implementation suggested by Ulf Hansson is using a
 * single global power domain and implement the ->attach|detach_dev()
 * callback for the genpd and use the regular of_genpd_add_provider_simple().
 * From within the ->attach_dev(), we could get the OF node for
 * the device that is being attached and then parse the power-domain
 * cell containing the "resource id" and store that in the per device
 * struct generic_pm_domain_data (we have void pointer there for
 * storing these kind of things).
 *
 * Additionally, we need to implement the ->stop() and ->start()
 * callbacks of genpd, which is where you "power on/off" devices,
 * rather than using the above ->power_on|off() callbacks.
 *
 * However, there're two known issues:
 * 1. The ->attach_dev() of power domain infrastructure still does
 *    not support multi domains case as the struct device *dev passed
 *    in is a virtual PD device, it does not help for parsing the real
 *    device resource id from device tree, so it's unware of which
 *    real sub power domain of device should be attached.
 *
 *    The framework needs some proper extension to support multi power
 *    domain cases.
 *
 *    Update: Genpd assigns the ->of_node for the virtual device before it
 *    invokes ->attach_dev() callback, hence parsing for device resources via
 *    DT should work fine.
 *
 * 2. It also breaks most of current drivers as the driver probe sequence
 *    behavior changed if removing ->power_on|off() callback and use
 *    ->start() and ->stop() instead. genpd_dev_pm_attach will only power
 *    up the domain and attach device, but will not call .start() which
 *    relies on device runtime pm. That means the device power is still
 *    not up before running driver probe function. For SCU enabled
 *    platforms, all device drivers accessing registers/clock without power
 *    domain enabled will trigger a HW access error. That means we need fix
 *    most drivers probe sequence with proper runtime pm.
 *
 *    Update: Runtime PM support isn't necessary. Instead, this can easily be
 *    fixed in drivers by adding a call to dev_pm_domain_start() during probe.
 *
 * In summary, the second part needs to be addressed via minor updates to the
 * relevant drivers, before the "single global power domain" model can be used.
 *
 */

#include <linux/arm-smccc.h>
#include <dt-bindings/firmware/imx/rsrc.h>
#include <linux/console.h>
#include <linux/firmware/imx/sci.h>
#include <linux/firmware/imx/svc/rm.h>
#include <linux/io.h>
#include <linux/irqchip/arm-gic-v3.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/pm_domain.h>
#include <linux/slab.h>
#include <linux/syscore_ops.h>

#define IMX_WU_MAX_IRQS	(((IMX_SC_R_LAST + 31) / 32 ) * 32 )

#define IMX_SIP_WAKEUP_SRC              0xc2000009
#define IMX_SIP_WAKEUP_SRC_SCU          0x1
#define IMX_SIP_WAKEUP_SRC_IRQSTEER     0x2

static u32 wu[IMX_WU_MAX_IRQS];
static int wu_num;
static void __iomem *gic_dist_base;

/* SCU Power Mode Protocol definition */
struct imx_sc_msg_req_set_resource_power_mode {
	struct imx_sc_rpc_msg hdr;
	u16 resource;
	u8 mode;
} __packed __aligned(4);

struct req_get_resource_mode {
	u16 resource;
};

struct resp_get_resource_mode {
	u8 mode;
};

struct imx_sc_msg_req_get_resource_power_mode {
	struct imx_sc_rpc_msg hdr;
	union {
		struct req_get_resource_mode req;
		struct resp_get_resource_mode resp;
	} data;
} __packed __aligned(4);

#define IMX_SCU_PD_NAME_SIZE 20
struct imx_sc_pm_domain {
	struct generic_pm_domain pd;
	char name[IMX_SCU_PD_NAME_SIZE];
	u32 rsrc;
};

struct imx_sc_pd_range {
	char *name;
	u32 rsrc;
	u8 num;

	/* add domain index */
	bool postfix;
	u8 start_from;
};

struct imx_sc_pd_soc {
	const struct imx_sc_pd_range *pd_ranges;
	u8 num_ranges;
};

static int imx_con_rsrc;

/* Align with the IMX_SC_PM_PW_MODE_[OFF,STBY,LP,ON] macros */
static const char * const imx_sc_pm_mode[] = {
	"IMX_SC_PM_PW_MODE_OFF",
	"IMX_SC_PM_PW_MODE_STBY",
	"IMX_SC_PM_PW_MODE_LP",
	"IMX_SC_PM_PW_MODE_ON"
};

static const struct imx_sc_pd_range imx8qxp_scu_pd_ranges[] = {
	/* LSIO SS */
	{ "pwm", IMX_SC_R_PWM_0, 8, true, 0 },
	{ "gpio", IMX_SC_R_GPIO_0, 8, true, 0 },
	{ "gpt", IMX_SC_R_GPT_0, 5, true, 0 },
	{ "kpp", IMX_SC_R_KPP, 1, false, 0 },
	{ "fspi", IMX_SC_R_FSPI_0, 2, true, 0 },
	{ "mu_a", IMX_SC_R_MU_0A, 14, true, 0 },
	{ "mu_b", IMX_SC_R_MU_5B, 9, true, 5 },

	/* CONN SS */
	{ "usb", IMX_SC_R_USB_0, 2, true, 0 },
	{ "usb0phy", IMX_SC_R_USB_0_PHY, 1, false, 0 },
	{ "usb1phy", IMX_SC_R_USB_1_PHY, 1, false, 0},
	{ "usb2", IMX_SC_R_USB_2, 1, false, 0 },
	{ "usb2phy", IMX_SC_R_USB_2_PHY, 1, false, 0 },
	{ "sdhc", IMX_SC_R_SDHC_0, 3, true, 0 },
	{ "enet", IMX_SC_R_ENET_0, 2, true, 0 },
	{ "nand", IMX_SC_R_NAND, 1, false, 0 },

	/* AUDIO SS */
	{ "audio-pll0", IMX_SC_R_AUDIO_PLL_0, 1, false, 0 },
	{ "audio-pll1", IMX_SC_R_AUDIO_PLL_1, 1, false, 0 },
	{ "audio-clk-0", IMX_SC_R_AUDIO_CLK_0, 1, false, 0 },
	{ "audio-clk-1", IMX_SC_R_AUDIO_CLK_1, 1, false, 0 },
	{ "mclk-out-0", IMX_SC_R_MCLK_OUT_0, 1, false, 0 },
	{ "mclk-out-1", IMX_SC_R_MCLK_OUT_1, 1, false, 0 },
	{ "dma0-ch", IMX_SC_R_DMA_0_CH0, 32, true, 0 },
	{ "dma1-ch", IMX_SC_R_DMA_1_CH0, 16, true, 0 },
	{ "dma2-ch-0", IMX_SC_R_DMA_2_CH0, 5, true, 0 },
	{ "dma2-ch-1", IMX_SC_R_DMA_2_CH5, 27, true, 0 },
	{ "dma3-ch", IMX_SC_R_DMA_3_CH0, 32, true, 0 },
	{ "asrc0", IMX_SC_R_ASRC_0, 1, false, 0 },
	{ "asrc1", IMX_SC_R_ASRC_1, 1, false, 0 },
	{ "esai0", IMX_SC_R_ESAI_0, 1, false, 0 },
	{ "esai1", IMX_SC_R_ESAI_1, 1, false, 0 },
	{ "spdif0", IMX_SC_R_SPDIF_0, 1, false, 0 },
	{ "spdif1", IMX_SC_R_SPDIF_1, 1, false, 0 },
	{ "sai", IMX_SC_R_SAI_0, 3, true, 0 },
	{ "sai3", IMX_SC_R_SAI_3, 1, false, 0 },
	{ "sai4", IMX_SC_R_SAI_4, 1, false, 0 },
	{ "sai5", IMX_SC_R_SAI_5, 1, false, 0 },
	{ "sai6", IMX_SC_R_SAI_6, 1, false, 0 },
	{ "sai7", IMX_SC_R_SAI_7, 1, false, 0 },
	{ "amix", IMX_SC_R_AMIX, 1, false, 0 },
	{ "mqs0", IMX_SC_R_MQS_0, 1, false, 0 },
	{ "dsp", IMX_SC_R_DSP, 1, false, 0 },
	{ "dsp-ram", IMX_SC_R_DSP_RAM, 1, false, 0 },

	/* DMA SS */
	{ "can", IMX_SC_R_CAN_0, 3, true, 0 },
	{ "ftm", IMX_SC_R_FTM_0, 2, true, 0 },
	{ "lpi2c", IMX_SC_R_I2C_0, 5, true, 0 },
	{ "adc", IMX_SC_R_ADC_0, 2, true, 0 },
	{ "lcd", IMX_SC_R_LCD_0, 1, true, 0 },
	{ "lcd-pll", IMX_SC_R_ELCDIF_PLL, 1, true, 0 },
	{ "lcd0-pwm", IMX_SC_R_LCD_0_PWM_0, 1, true, 0 },
	{ "lpuart", IMX_SC_R_UART_0, 5, true, 0 },
	{ "sim", IMX_SC_R_EMVSIM_0, 2, true, 0 },
	{ "lpspi", IMX_SC_R_SPI_0, 4, true, 0 },
	{ "irqstr_dsp", IMX_SC_R_IRQSTR_DSP, 1, false, 0 },

	/* VPU SS */
	{ "vpu", IMX_SC_R_VPU, 1, false, 0 },
	{ "vpu-pid", IMX_SC_R_VPU_PID0, 8, true, 0 },
	{ "vpu-dec0", IMX_SC_R_VPU_DEC_0, 1, false, 0 },
	{ "vpu-enc0", IMX_SC_R_VPU_ENC_0, 1, false, 0 },
	{ "vpu-enc1", IMX_SC_R_VPU_ENC_1, 1, false, 0 },
	{ "vpu-mu0", IMX_SC_R_VPU_MU_0, 1, false, 0 },
	{ "vpu-mu1", IMX_SC_R_VPU_MU_1, 1, false, 0 },
	{ "vpu-mu2", IMX_SC_R_VPU_MU_2, 1, false, 0 },

	/* GPU SS */
	{ "gpu0-pid", IMX_SC_R_GPU_0_PID0, 4, true, 0 },
	{ "gpu1-pid", IMX_SC_R_GPU_1_PID0, 4, true, 0 },


	/* HSIO SS */
	{ "pcie-a", IMX_SC_R_PCIE_A, 1, false, 0 },
	{ "serdes-0", IMX_SC_R_SERDES_0, 1, false, 0 },
	{ "pcie-b", IMX_SC_R_PCIE_B, 1, false, 0 },
	{ "serdes-1", IMX_SC_R_SERDES_1, 1, false, 0 },
	{ "sata-0", IMX_SC_R_SATA_0, 1, false, 0 },
	{ "hsio-gpio", IMX_SC_R_HSIO_GPIO, 1, false, 0 },

	/* MIPI SS */
	{ "mipi0", IMX_SC_R_MIPI_0, 1, false, 0 },
	{ "mipi0-pwm0", IMX_SC_R_MIPI_0_PWM_0, 1, false, 0 },
	{ "mipi0-i2c", IMX_SC_R_MIPI_0_I2C_0, 2, true, 0 },

	{ "mipi1", IMX_SC_R_MIPI_1, 1, false, 0 },
	{ "mipi1-pwm0", IMX_SC_R_MIPI_1_PWM_0, 1, false, 0 },
	{ "mipi1-i2c", IMX_SC_R_MIPI_1_I2C_0, 2, true, 0 },

	/* LVDS SS */
	{ "lvds0", IMX_SC_R_LVDS_0, 1, false, 0 },
	{ "lvds0-pwm", IMX_SC_R_LVDS_0_PWM_0, 1, false, 0 },
	{ "lvds0-lpi2c", IMX_SC_R_LVDS_0_I2C_0, 2, true, 0 },
	{ "lvds1", IMX_SC_R_LVDS_1, 1, false, 0 },
	{ "lvds1-pwm", IMX_SC_R_LVDS_1_PWM_0, 1, false, 0 },
	{ "lvds1-lpi2c", IMX_SC_R_LVDS_1_I2C_0, 2, true, 0 },

	/* DC SS */
	{ "dc0", IMX_SC_R_DC_0, 1, false, 0 },
	{ "dc0-pll", IMX_SC_R_DC_0_PLL_0, 2, true, 0 },
	{ "dc0-video", IMX_SC_R_DC_0_VIDEO0, 2, true, 0 },

	{ "dc1", IMX_SC_R_DC_1, 1, false, 0 },
	{ "dc1-pll", IMX_SC_R_DC_1_PLL_0, 2, true, 0 },
	{ "dc1-video", IMX_SC_R_DC_1_VIDEO0, 2, true, 0 },

	/* CM40 SS */
	{ "cm40-i2c", IMX_SC_R_M4_0_I2C, 1, false, 0 },
	{ "cm40-intmux", IMX_SC_R_M4_0_INTMUX, 1, false, 0 },
	{ "cm40-pid", IMX_SC_R_M4_0_PID0, 5, true, 0},
	{ "cm40-mu-a1", IMX_SC_R_M4_0_MU_1A, 1, false, 0},
	{ "cm40-lpuart", IMX_SC_R_M4_0_UART, 1, false, 0},

	/* CM41 SS */
	{ "cm41-i2c", IMX_SC_R_M4_1_I2C, 1, false, 0 },
	{ "cm41-intmux", IMX_SC_R_M4_1_INTMUX, 1, false, 0 },
	{ "cm41-pid", IMX_SC_R_M4_1_PID0, 5, true, 0},
	{ "cm41-mu-a1", IMX_SC_R_M4_1_MU_1A, 1, false, 0},
	{ "cm41-lpuart", IMX_SC_R_M4_1_UART, 1, false, 0},

	/* CM41 SS */
	{ "cm41_i2c", IMX_SC_R_M4_1_I2C, 1, false, 0 },
	{ "cm41_intmux", IMX_SC_R_M4_1_INTMUX, 1, false, 0 },

	/* DB SS */
	{ "perf", IMX_SC_R_PERF, 1, false, 0},

	/* IMAGE SS */
	{ "img-jpegdec-mp", IMX_SC_R_MJPEG_DEC_MP, 1, false, 0 },
	{ "img-jpegdec-s0", IMX_SC_R_MJPEG_DEC_S0, 4, true, 0 },
	{ "img-jpegenc-mp", IMX_SC_R_MJPEG_ENC_MP, 1, false, 0 },
	{ "img-jpegenc-s0", IMX_SC_R_MJPEG_ENC_S0, 4, true, 0 },

	/* SECO SS */
	{ "seco_mu", IMX_SC_R_SECO_MU_2, 3, true, 2},

	/* V2X SS */
	{ "v2x_mu", IMX_SC_R_V2X_MU_0, 2, true, 0},
	{ "v2x_mu", IMX_SC_R_V2X_MU_2, 1, true, 2},
	{ "v2x_mu", IMX_SC_R_V2X_MU_3, 2, true, 3},
	{ "img-pdma", IMX_SC_R_ISI_CH0, 8, true, 0 },
	{ "img-csi0", IMX_SC_R_CSI_0, 1, false, 0 },
	{ "img-csi0-i2c0", IMX_SC_R_CSI_0_I2C_0, 1, false, 0 },
	{ "img-csi0-pwm0", IMX_SC_R_CSI_0_PWM_0, 1, false, 0 },
	{ "img-csi1", IMX_SC_R_CSI_1, 1, false, 0 },
	{ "img-csi1-i2c0", IMX_SC_R_CSI_1_I2C_0, 1, false, 0 },
	{ "img-csi1-pwm0", IMX_SC_R_CSI_1_PWM_0, 1, false, 0 },
	{ "img-parallel", IMX_SC_R_PI_0, 1, false, 0 },
	{ "img-parallel-i2c0", IMX_SC_R_PI_0_I2C_0, 1, false, 0 },
	{ "img-parallel-pwm0", IMX_SC_R_PI_0_PWM_0, 2, true, 0 },
	{ "img-parallel-pll", IMX_SC_R_PI_0_PLL, 1, false, 0 },

	/* HDMI TX SS */
	{ "hdmi-tx", IMX_SC_R_HDMI, 1, false, 0},
	{ "hdmi-tx-i2s", IMX_SC_R_HDMI_I2S, 1, false, 0},
	{ "hdmi-tx-i2c0", IMX_SC_R_HDMI_I2C_0, 1, false, 0},
	{ "hdmi-tx-pll0", IMX_SC_R_HDMI_PLL_0, 1, false, 0},
	{ "hdmi-tx-pll1", IMX_SC_R_HDMI_PLL_1, 1, false, 0},

	/* HDMI RX SS */
	{ "hdmi-rx", IMX_SC_R_HDMI_RX, 1, false, 0},
	{ "hdmi-rx-pwm", IMX_SC_R_HDMI_RX_PWM_0, 1, false, 0},
	{ "hdmi-rx-i2c0", IMX_SC_R_HDMI_RX_I2C_0, 1, false, 0},
	{ "hdmi-rx-bypass", IMX_SC_R_HDMI_RX_BYPASS, 1, false, 0},

	/* SECURITY SS */
	{ "sec-jr", IMX_SC_R_CAAM_JR2, 2, true, 2},

	/* BOARD SS */
	{ "board", IMX_SC_R_BOARD_R0, 8, true, 0},
};

static const struct imx_sc_pd_soc imx8qxp_scu_pd = {
	.pd_ranges = imx8qxp_scu_pd_ranges,
	.num_ranges = ARRAY_SIZE(imx8qxp_scu_pd_ranges),
};

static struct imx_sc_ipc *pm_ipc_handle;

static inline struct imx_sc_pm_domain *
to_imx_sc_pd(struct generic_pm_domain *genpd)
{
	return container_of(genpd, struct imx_sc_pm_domain, pd);
}

static int imx_pm_domains_suspend(void)
{
	struct arm_smccc_res res;
	u32 offset;
	int i;

	for (i = 0; i < wu_num; i++) {
		offset = GICD_ISENABLER + ((wu[i] + 32) / 32) * 4;
		if (BIT(wu[i] % 32) & readl_relaxed(gic_dist_base + offset)) {
			arm_smccc_smc(IMX_SIP_WAKEUP_SRC,
				      IMX_SIP_WAKEUP_SRC_IRQSTEER,
				      0, 0, 0, 0, 0, 0, &res);
			return 0;
		}
	}

	arm_smccc_smc(IMX_SIP_WAKEUP_SRC,
		      IMX_SIP_WAKEUP_SRC_SCU,
		      0, 0, 0, 0, 0, 0, &res);

	return 0;
}

struct syscore_ops imx_pm_domains_syscore_ops = {
	.suspend = imx_pm_domains_suspend,
};

static void imx_sc_pd_enable_irqsteer_wakeup(struct device_node *np)
{
	struct device_node *gic_node;
	unsigned int i;

	wu_num = of_property_count_u32_elems(np, "wakeup-irq");
	if (wu_num <= 0) {
		pr_warn("no irqsteer wakeup source supported!\n");
		return;
	}

	gic_node = of_find_compatible_node(NULL, NULL, "arm,gic-v3");
	WARN_ON(!gic_node);

	gic_dist_base = of_iomap(gic_node, 0);
	WARN_ON(!gic_dist_base);

	for (i = 0; i < wu_num; i++)
		WARN_ON(of_property_read_u32_index(np, "wakeup-irq", i, &wu[i]));

	register_syscore_ops(&imx_pm_domains_syscore_ops);
}

static void imx_sc_pd_get_console_rsrc(void)
{
	struct of_phandle_args specs;
	int ret;

	if (!of_stdout)
		return;

	ret = of_parse_phandle_with_args(of_stdout, "power-domains",
					 "#power-domain-cells",
					 0, &specs);
	if (ret)
		return;

	imx_con_rsrc = specs.args[0];
}

static int imx_sc_get_pd_power(struct device *dev, u32 rsrc)
{
	struct imx_sc_msg_req_get_resource_power_mode msg;
	struct imx_sc_rpc_msg *hdr = &msg.hdr;
	int ret;

	hdr->ver = IMX_SC_RPC_VERSION;
	hdr->svc = IMX_SC_RPC_SVC_PM;
	hdr->func = IMX_SC_PM_FUNC_GET_RESOURCE_POWER_MODE;
	hdr->size = 2;

	msg.data.req.resource = rsrc;

	ret = imx_scu_call_rpc(pm_ipc_handle, &msg, true);
	if (ret)
		dev_err(dev, "failed to get power resource %d mode, ret %d\n",
			rsrc, ret);

	return msg.data.resp.mode;
}

static int imx_sc_pd_power(struct generic_pm_domain *domain, bool power_on)
{
	struct imx_sc_msg_req_set_resource_power_mode msg;
	struct imx_sc_rpc_msg *hdr = &msg.hdr;
	struct imx_sc_pm_domain *pd;
	int ret;

	pd = to_imx_sc_pd(domain);

	hdr->ver = IMX_SC_RPC_VERSION;
	hdr->svc = IMX_SC_RPC_SVC_PM;
	hdr->func = IMX_SC_PM_FUNC_SET_RESOURCE_POWER_MODE;
	hdr->size = 2;

	msg.resource = pd->rsrc;
	msg.mode = power_on ? IMX_SC_PM_PW_MODE_ON : pd->pd.state_idx ?
		   IMX_SC_PM_PW_MODE_OFF : IMX_SC_PM_PW_MODE_LP;

	/* keep uart console power on for no_console_suspend */
	if (imx_con_rsrc == pd->rsrc && !console_suspend_enabled && !power_on)
		return -EBUSY;

	ret = imx_scu_call_rpc(pm_ipc_handle, &msg, true);
	if (ret == -EACCES)
	{
		pr_warn("Resource %d not owned by partition, power state unchanged\n",
			pd->rsrc);
		return 0;
	}
	if (ret)
		dev_err(&domain->dev, "failed to power %s resource %d ret %d\n",
			power_on ? "up" : "off", pd->rsrc, ret);

	return ret;
}

static int imx_sc_pd_power_on(struct generic_pm_domain *domain)
{
	return imx_sc_pd_power(domain, true);
}

static int imx_sc_pd_power_off(struct generic_pm_domain *domain)
{
	return imx_sc_pd_power(domain, false);
}

static struct generic_pm_domain *imx_scu_pd_xlate(const struct of_phandle_args *spec,
						  void *data)
{
	struct generic_pm_domain *domain = ERR_PTR(-ENOENT);
	struct genpd_onecell_data *pd_data = data;
	unsigned int i;

	for (i = 0; i < pd_data->num_domains; i++) {
		struct imx_sc_pm_domain *sc_pd;

		sc_pd = to_imx_sc_pd(pd_data->domains[i]);
		if (sc_pd->rsrc == spec->args[0]) {
			domain = &sc_pd->pd;
			break;
		}
	}

	return domain;
}

static struct imx_sc_pm_domain *
imx_scu_add_pm_domain(struct device *dev, int idx,
		      const struct imx_sc_pd_range *pd_ranges)
{
	struct imx_sc_pm_domain *sc_pd;
	struct genpd_power_state *states;
	bool is_off;
	int mode, ret;

	if (!imx_sc_rm_is_resource_owned(pm_ipc_handle, pd_ranges->rsrc + idx))
		return NULL;

	sc_pd = devm_kzalloc(dev, sizeof(*sc_pd), GFP_KERNEL);
	if (!sc_pd)
		return ERR_PTR(-ENOMEM);

	states = devm_kcalloc(dev, 2, sizeof(*states), GFP_KERNEL);
	if (!states) {
		devm_kfree(dev, sc_pd);
		return ERR_PTR(-ENOMEM);
	}

	sc_pd->rsrc = pd_ranges->rsrc + idx;
	sc_pd->pd.power_off = imx_sc_pd_power_off;
	sc_pd->pd.power_on = imx_sc_pd_power_on;
	sc_pd->pd.flags |= GENPD_FLAG_ACTIVE_WAKEUP;
	states[0].power_off_latency_ns = 25000;
	states[0].power_on_latency_ns =  25000;
	states[1].power_off_latency_ns = 2500000;
	states[1].power_on_latency_ns =  2500000;

	sc_pd->pd.states = states;
	sc_pd->pd.state_count = 2;

	if (pd_ranges->postfix)
		snprintf(sc_pd->name, sizeof(sc_pd->name),
			 "%s%i", pd_ranges->name, pd_ranges->start_from + idx);
	else
		snprintf(sc_pd->name, sizeof(sc_pd->name),
			 "%s", pd_ranges->name);

	sc_pd->pd.name = sc_pd->name;
	if (imx_con_rsrc == sc_pd->rsrc)
		sc_pd->pd.flags |= GENPD_FLAG_RPM_ALWAYS_ON;

	mode = imx_sc_get_pd_power(dev, pd_ranges->rsrc + idx);
	if (mode == IMX_SC_PM_PW_MODE_ON)
		is_off = false;
	else
		is_off = true;

	dev_dbg(dev, "%s : %s\n", sc_pd->name, imx_sc_pm_mode[mode]);

	if (sc_pd->rsrc >= IMX_SC_R_LAST) {
		dev_warn(dev, "invalid pd %s rsrc id %d found",
			 sc_pd->name, sc_pd->rsrc);

		devm_kfree(dev, sc_pd);
		devm_kfree(dev, states);
		return NULL;
	}

	ret = pm_genpd_init(&sc_pd->pd, NULL, is_off);
	if (ret) {
		dev_warn(dev, "failed to init pd %s rsrc id %d",
			 sc_pd->name, sc_pd->rsrc);
		devm_kfree(dev, sc_pd);
		devm_kfree(dev, states);
		return NULL;
	}

	return sc_pd;
}

static int imx_scu_init_pm_domains(struct device *dev,
				    const struct imx_sc_pd_soc *pd_soc)
{
	const struct imx_sc_pd_range *pd_ranges = pd_soc->pd_ranges;
	struct generic_pm_domain **domains;
	struct genpd_onecell_data *pd_data;
	struct imx_sc_pm_domain *sc_pd;
	u32 count = 0;
	int i, j;

	for (i = 0; i < pd_soc->num_ranges; i++)
		count += pd_ranges[i].num;

	domains = devm_kcalloc(dev, count, sizeof(*domains), GFP_KERNEL);
	if (!domains)
		return -ENOMEM;

	pd_data = devm_kzalloc(dev, sizeof(*pd_data), GFP_KERNEL);
	if (!pd_data)
		return -ENOMEM;

	count = 0;
	for (i = 0; i < pd_soc->num_ranges; i++) {
		for (j = 0; j < pd_ranges[i].num; j++) {
			sc_pd = imx_scu_add_pm_domain(dev, j, &pd_ranges[i]);
			if (IS_ERR_OR_NULL(sc_pd))
				continue;

			domains[count++] = &sc_pd->pd;
			dev_dbg(dev, "added power domain %s\n", sc_pd->pd.name);
		}
	}

	pd_data->domains = domains;
	pd_data->num_domains = count;
	pd_data->xlate = imx_scu_pd_xlate;

	of_genpd_add_provider_onecell(dev->of_node, pd_data);

	return 0;
}

static int imx_sc_pd_probe(struct platform_device *pdev)
{
	const struct imx_sc_pd_soc *pd_soc;
	int ret;

	ret = imx_scu_get_handle(&pm_ipc_handle);
	if (ret)
		return ret;

	pd_soc = of_device_get_match_data(&pdev->dev);
	if (!pd_soc)
		return -ENODEV;

	imx_sc_pd_get_console_rsrc();
	imx_sc_pd_enable_irqsteer_wakeup(pdev->dev.of_node);

	return imx_scu_init_pm_domains(&pdev->dev, pd_soc);
}

static const struct of_device_id imx_sc_pd_match[] = {
	{ .compatible = "fsl,imx8qxp-scu-pd", &imx8qxp_scu_pd},
	{ .compatible = "fsl,scu-pd", &imx8qxp_scu_pd},
	{ /* sentinel */ }
};

static struct platform_driver imx_sc_pd_driver = {
	.driver = {
		.name = "imx-scu-pd",
		.of_match_table = imx_sc_pd_match,
		.suppress_bind_attrs = true,
	},
	.probe = imx_sc_pd_probe,
};

static int __init imx_sc_pd_driver_init(void)
{
	return platform_driver_register(&imx_sc_pd_driver);
}
subsys_initcall(imx_sc_pd_driver_init);

MODULE_AUTHOR("Dong Aisheng <aisheng.dong@nxp.com>");
MODULE_DESCRIPTION("IMX SCU Power Domain driver");
MODULE_LICENSE("GPL v2");
