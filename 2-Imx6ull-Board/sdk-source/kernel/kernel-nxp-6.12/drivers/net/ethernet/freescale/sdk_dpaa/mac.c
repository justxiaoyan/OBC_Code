/* Copyright 2008-2012 Freescale Semiconductor, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *	 notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *	 notice, this list of conditions and the following disclaimer in the
 *	 documentation and/or other materials provided with the distribution.
 *     * Neither the name of Freescale Semiconductor nor the
 *	 names of its contributors may be used to endorse or promote products
 *	 derived from this software without specific prior written permission.
 *
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") as published by the Free Software
 * Foundation, either version 2 of that License or (at your option) any
 * later version.
 *
 * THIS SOFTWARE IS PROVIDED BY Freescale Semiconductor ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Freescale Semiconductor BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef CONFIG_FSL_DPAA_ETH_DEBUG
#define pr_fmt(fmt) \
	KBUILD_MODNAME ": %s:%hu:%s() " fmt, \
	KBUILD_BASENAME".c", __LINE__, __func__
#else
#define pr_fmt(fmt) \
	KBUILD_MODNAME ": " fmt
#endif

#include <linux/init.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_net.h>
#include <linux/of_mdio.h>
#include <linux/phy_fixed.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/phy.h>
#include <linux/phylink.h>
#include <linux/io.h>

#include "lnxwrp_fm_ext.h"

#include "mac.h"

#define MEMAC_SUPPORTED \
	(SUPPORTED_10baseT_Half \
	| SUPPORTED_10baseT_Full \
	| SUPPORTED_100baseT_Half \
	| SUPPORTED_100baseT_Full \
	| SUPPORTED_1000baseKX_Full \
	| SUPPORTED_10000baseKR_Full \
	| SUPPORTED_Autoneg \
	| SUPPORTED_Pause \
	| SUPPORTED_Asym_Pause \
	| SUPPORTED_FIBRE \
	| SUPPORTED_MII \
	| SUPPORTED_Backplane)

static int __cold free_macdev(struct mac_device *mac_dev)
{
	dev_set_drvdata(mac_dev->dev, NULL);

	return mac_dev->uninit(mac_dev->get_mac_handle(mac_dev));
}

static const struct of_device_id mac_match[] = {
	{ .compatible	= "fsl,fman-memac" },
	{}
};
MODULE_DEVICE_TABLE(of, mac_match);

static int __cold mac_probe(struct platform_device *_of_dev)
{
	int			 _errno, i;
	struct device		*dev;
	struct device_node	*mac_node, *dev_node;
	struct mac_device	*mac_dev;
	struct platform_device	*of_dev;
	struct resource		 res;
	int			nph;
	u32			cell_index;
	const struct of_device_id *match;

	dev = &_of_dev->dev;
	mac_node = dev->of_node;

	match = of_match_device(mac_match, dev);
	if (!match)
		return -EINVAL;

	for (i = 0; i < ARRAY_SIZE(mac_match) - 1 && match != mac_match + i;
			i++)
		;
	BUG_ON(i >= ARRAY_SIZE(mac_match) - 1);

	mac_dev = memac_alloc(dev);
	if (IS_ERR(mac_dev)) {
		_errno = PTR_ERR(mac_dev);
		dev_err(dev, "memac_alloc() = %d\n", _errno);
		goto _return;
	}

	INIT_LIST_HEAD(&mac_dev->mc_addr_list);

	/* Get the FM node */
	dev_node = of_get_parent(mac_node);
	if (unlikely(dev_node == NULL)) {
		dev_err(dev, "of_get_parent(%s) failed\n",
				mac_node->full_name);
		_errno = -EINVAL;
		goto _return_dev_set_drvdata;
	}

	of_dev = of_find_device_by_node(dev_node);
	if (unlikely(of_dev == NULL)) {
		dev_err(dev, "of_find_device_by_node(%s) failed\n",
				dev_node->full_name);
		_errno = -EINVAL;
		goto _return_of_node_put;
	}

	mac_dev->fm_dev = fm_bind(&of_dev->dev);
	if (unlikely(mac_dev->fm_dev == NULL)) {
		dev_err(dev, "fm_bind(%s) failed\n", dev_node->full_name);
		_errno = -ENODEV;
		goto _return_of_node_put;
	}

	mac_dev->fm = (void *)fm_get_handle(mac_dev->fm_dev);
	of_node_put(dev_node);

	/* Get the address of the memory mapped registers */
	_errno = of_address_to_resource(mac_node, 0, &res);
	if (unlikely(_errno < 0)) {
		dev_err(dev, "of_address_to_resource(%s) = %d\n",
				mac_node->full_name, _errno);
		goto _return_dev_set_drvdata;
	}

	mac_dev->res = __devm_request_region(
		dev,
		fm_get_mem_region(mac_dev->fm_dev),
		res.start, res.end + 1 - res.start, "mac");
	if (unlikely(mac_dev->res == NULL)) {
		dev_err(dev, "__devm_request_mem_region(mac) failed\n");
		_errno = -EBUSY;
		goto _return_dev_set_drvdata;
	}

	mac_dev->vaddr = devm_ioremap(dev, mac_dev->res->start,
				      mac_dev->res->end + 1
				      - mac_dev->res->start);
	if (unlikely(mac_dev->vaddr == NULL)) {
		dev_err(dev, "devm_ioremap() failed\n");
		_errno = -EIO;
		goto _return_dev_set_drvdata;
	}

#define TBIPA_OFFSET		0x1c
#define TBIPA_DEFAULT_ADDR	5 /* override if used as external PHY addr. */
	mac_dev->tbi_node = of_parse_phandle(mac_node, "tbi-handle", 0);
	if (mac_dev->tbi_node) {
		u32 tbiaddr = TBIPA_DEFAULT_ADDR;
		const __be32 *tbi_reg;
		void __iomem *addr;

		tbi_reg = of_get_property(mac_dev->tbi_node, "reg", NULL);
		if (tbi_reg)
			tbiaddr = be32_to_cpup(tbi_reg);
		addr = mac_dev->vaddr + TBIPA_OFFSET;
		/* TODO: out_be32 does not exist on ARM */
		out_be32(addr, tbiaddr);
	}

	if (!of_device_is_available(mac_node)) {
		devm_iounmap(dev, mac_dev->vaddr);
		__devm_release_region(dev, fm_get_mem_region(mac_dev->fm_dev),
			res.start, res.end + 1 - res.start);
		fm_unbind(mac_dev->fm_dev);
		devm_kfree(dev, mac_dev);
		dev_set_drvdata(dev, NULL);
		return -ENODEV;
	}

	/* Get the cell-index */
	_errno = of_property_read_u32(mac_node, "cell-index", &cell_index);
	if (unlikely(_errno)) {
		dev_err(dev, "Cannot read cell-index of mac node %s from device tree\n",
				mac_node->full_name);
		goto _return_dev_set_drvdata;
	}
	mac_dev->cell_index = (uint8_t)cell_index;
	if (mac_dev->cell_index >= 8)
		mac_dev->cell_index -= 8;

	/* Get the MAC address */
	_errno = of_get_mac_address(mac_node, mac_dev->addr);
	if (unlikely(_errno)) {
		dev_err(dev, "of_get_mac_address(%s) failed\n",
				mac_node->full_name);
		_errno = -EINVAL;
		goto _return_dev_set_drvdata;
	}

	/* Verify the number of port handles */
	nph = of_count_phandle_with_args(mac_node, "fsl,fman-ports", NULL);
	if (unlikely(nph < 0)) {
		dev_err(dev, "Cannot read port handles of mac node %s from device tree\n",
				mac_node->full_name);
		_errno = nph;
		goto _return_dev_set_drvdata;
	}

	if (nph != ARRAY_SIZE(mac_dev->port_dev)) {
		dev_err(dev, "Not supported number of port handles of mac node %s from device tree\n",
				mac_node->full_name);
		_errno = -EINVAL;
		goto _return_dev_set_drvdata;
	}

	for_each_port_device(i, mac_dev->port_dev) {
		dev_node = of_parse_phandle(mac_node, "fsl,fman-ports", i);
		if (unlikely(dev_node == NULL)) {
			dev_err(dev, "Cannot find port node referenced by mac node %s from device tree\n",
					mac_node->full_name);
			_errno = -EINVAL;
			goto _return_of_node_put;
		}

		of_dev = of_find_device_by_node(dev_node);
		if (unlikely(of_dev == NULL)) {
			dev_err(dev, "of_find_device_by_node(%s) failed\n",
					dev_node->full_name);
			_errno = -EINVAL;
			goto _return_of_node_put;
		}

		mac_dev->port_dev[i] = fm_port_bind(&of_dev->dev);
		if (unlikely(mac_dev->port_dev[i] == NULL)) {
			dev_err(dev, "dev_get_drvdata(%s) failed\n",
					dev_node->full_name);
			_errno = -EINVAL;
			goto _return_of_node_put;
		}
		of_node_put(dev_node);
	}

	/* Get the PHY connection type */
	_errno = of_get_phy_mode(mac_node, &mac_dev->phy_if);
	if (unlikely(_errno)) {
		dev_warn(dev,
			 "Cannot read PHY connection type of mac node %s from device tree. Defaulting to MII\n",
			 mac_node->full_name);
		mac_dev->phy_if = PHY_INTERFACE_MODE_MII;
	}

	mac_dev->link		= false;
	mac_dev->half_duplex	= false;
	mac_dev->speed		= phylink_interface_max_speed(mac_dev->phy_if);
	mac_dev->max_speed	= mac_dev->speed;
	mac_dev->if_support = MEMAC_SUPPORTED;
	/* We don't support half-duplex in SGMII mode */
	if (mac_dev->phy_if == PHY_INTERFACE_MODE_SGMII ||
	    mac_dev->phy_if == PHY_INTERFACE_MODE_QSGMII ||
	    mac_dev->phy_if == PHY_INTERFACE_MODE_2500SGMII)
		mac_dev->if_support &= ~(SUPPORTED_10baseT_Half |
					SUPPORTED_100baseT_Half);

	/* Gigabit support (no half-duplex) */
	if (mac_dev->max_speed == SPEED_1000 ||
	    mac_dev->max_speed == SPEED_2500)
		mac_dev->if_support |= SUPPORTED_1000baseT_Full;

	/* The 10G interface only supports one mode */
	if (mac_dev->phy_if == PHY_INTERFACE_MODE_XGMII)
		mac_dev->if_support = SUPPORTED_10000baseT_Full;

	/* The adjust_link() methods in this driver do not support the
	 * 1G <-> 10G MAC reconfiguration. Advertise a single technology
	 * ability through clause 73 auto-negotiation.
	 */
	if (mac_dev->phy_if == PHY_INTERFACE_MODE_10GKR)
		mac_dev->if_support &= ~SUPPORTED_1000baseKX_Full;
	if (mac_dev->phy_if == PHY_INTERFACE_MODE_1000BASEKX)
		mac_dev->if_support &= ~SUPPORTED_10000baseKR_Full;

	/* Get the rest of the PHY information */
	mac_dev->phy_node = of_parse_phandle(mac_node, "phy-handle", 0);
	if (!mac_dev->phy_node) {
		struct phy_device *phy;

		if (!of_phy_is_fixed_link(mac_node)) {
			dev_err(dev, "Wrong PHY information of mac node %s\n",
				mac_node->full_name);
			goto _return_dev_set_drvdata;
		}

		_errno = of_phy_register_fixed_link(mac_node);
		if (_errno)
			goto _return_dev_set_drvdata;

		mac_dev->fixed_link = devm_kzalloc(mac_dev->dev,
						   sizeof(*mac_dev->fixed_link),
						   GFP_KERNEL);
		if (!mac_dev->fixed_link)
			goto _return_dev_set_drvdata;

		mac_dev->phy_node = of_node_get(mac_node);
		phy = of_phy_find_device(mac_dev->phy_node);
		if (!phy)
			goto _return_dev_set_drvdata;

		mac_dev->fixed_link->link = phy->link;
		mac_dev->fixed_link->speed = phy->speed;
		mac_dev->fixed_link->duplex = phy->duplex;
		mac_dev->fixed_link->pause = phy->pause;
		mac_dev->fixed_link->asym_pause = phy->asym_pause;
	}

	_errno = mac_dev->init(mac_dev);
	if (unlikely(_errno < 0)) {
		dev_err(dev, "mac_dev->init() = %d\n", _errno);
		goto _return_dev_set_drvdata;
	}

	/* pause frame autonegotiation enabled*/
	mac_dev->autoneg_pause = true;

	/* by intializing the values to false, force FMD to enable PAUSE frames
	 * on RX and TX
	 */
	mac_dev->rx_pause_req = mac_dev->tx_pause_req = true;
	mac_dev->rx_pause_active = mac_dev->tx_pause_active = false;
	_errno = set_mac_active_pause(mac_dev, true, true);
	if (unlikely(_errno < 0))
		dev_err(dev, "set_mac_active_pause() = %d\n", _errno);

	dev_info(dev,
		"FMan MAC address: %02hx:%02hx:%02hx:%02hx:%02hx:%02hx\n",
		     mac_dev->addr[0], mac_dev->addr[1], mac_dev->addr[2],
		     mac_dev->addr[3], mac_dev->addr[4], mac_dev->addr[5]);

	goto _return;

_return_of_node_put:
	of_node_put(dev_node);
_return_dev_set_drvdata:
	dev_set_drvdata(dev, NULL);
_return:
	return _errno;
}

static void __cold mac_remove(struct platform_device *of_dev)
{
	struct device		*dev;
	struct mac_device	*mac_dev;
	int			 i;

	dev = &of_dev->dev;
	mac_dev = (struct mac_device *)dev_get_drvdata(dev);

	for_each_port_device(i, mac_dev->port_dev)
		fm_port_unbind(mac_dev->port_dev[i]);

	fm_unbind(mac_dev->fm_dev);

	free_macdev(mac_dev);
}

static struct platform_driver mac_driver = {
	.driver = {
		.name		= KBUILD_MODNAME,
		.of_match_table	= mac_match,
		.owner		= THIS_MODULE,
	},
	.probe		= mac_probe,
	.remove		= mac_remove
};

static int __init __cold mac_load(void)
{
	int	 _errno;

	pr_debug(KBUILD_MODNAME ": -> %s:%s()\n",
		KBUILD_BASENAME".c", __func__);

	pr_info(KBUILD_MODNAME ": %s\n", mac_driver_description);

	_errno = platform_driver_register(&mac_driver);
	if (unlikely(_errno < 0)) {
		pr_err(KBUILD_MODNAME ": %s:%hu:%s(): platform_driver_register() = %d\n",
			   KBUILD_BASENAME".c", __LINE__, __func__, _errno);
		goto _return;
	}

	goto _return;

_return:
	pr_debug(KBUILD_MODNAME ": %s:%s() ->\n",
		KBUILD_BASENAME".c", __func__);

	return _errno;
}
module_init(mac_load);

static void __exit __cold mac_unload(void)
{
	pr_debug(KBUILD_MODNAME ": -> %s:%s()\n",
		KBUILD_BASENAME".c", __func__);

	platform_driver_unregister(&mac_driver);

	pr_debug(KBUILD_MODNAME ": %s:%s() ->\n",
		KBUILD_BASENAME".c", __func__);
}
module_exit(mac_unload);
