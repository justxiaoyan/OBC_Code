// SPDX-License-Identifier: GPL-2.0-only
/*
 * fwnode helpers for the MDIO (Ethernet PHY) API
 *
 * This file provides helper functions for extracting PHY device information
 * out of the fwnode and using it to populate an mii_bus.
 */

#include <linux/acpi.h>
#include <linux/dev_printk.h>
#include <linux/fwnode_mdio.h>
#include <linux/of.h>
#include <linux/phy.h>
#include <linux/pse-pd/pse.h>

MODULE_AUTHOR("Calvin Johnson <calvin.johnson@oss.nxp.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("FWNODE MDIO bus (Ethernet PHY) accessors");

static struct pse_control *
fwnode_find_pse_control(struct fwnode_handle *fwnode)
{
	struct pse_control *psec;
	struct device_node *np;

	if (!IS_ENABLED(CONFIG_PSE_CONTROLLER))
		return NULL;

	np = to_of_node(fwnode);
	if (!np)
		return NULL;

	psec = of_pse_control_get(np);
	if (PTR_ERR(psec) == -ENOENT)
		return NULL;

	return psec;
}

static struct mii_timestamper *
fwnode_find_mii_timestamper(struct fwnode_handle *fwnode)
{
	struct of_phandle_args arg;
	int err;

	if (is_acpi_node(fwnode))
		return NULL;

	err = of_parse_phandle_with_fixed_args(to_of_node(fwnode),
					       "timestamper", 1, 0, &arg);
	if (err == -ENOENT)
		return NULL;
	else if (err)
		return ERR_PTR(err);

	if (arg.args_count != 1)
		return ERR_PTR(-EINVAL);

	return register_mii_timestamper(arg.np, arg.args[0]);
}

int fwnode_mdiobus_phy_device_register(struct mii_bus *mdio,
				       struct phy_device *phy,
				       struct fwnode_handle *child, u32 addr)
{
	int rc;

	rc = fwnode_irq_get(child, 0);
	/* Don't wait forever if the IRQ provider doesn't become available,
	 * just fall back to poll mode
	 */
	if (rc == -EPROBE_DEFER)
		rc = driver_deferred_probe_check_state(&phy->mdio.dev);
	if (rc == -EPROBE_DEFER)
		return rc;

	if (rc > 0) {
		phy->irq = rc;
		mdio->irq[addr] = rc;
	} else {
		phy->irq = mdio->irq[addr];
	}

	if (fwnode_property_read_bool(child, "broken-turn-around"))
		mdio->phy_ignore_ta_mask |= 1 << addr;

	fwnode_property_read_u32(child, "reset-assert-us",
				 &phy->mdio.reset_assert_delay);
	fwnode_property_read_u32(child, "reset-deassert-us",
				 &phy->mdio.reset_deassert_delay);

	/* Associate the fwnode with the device structure so it
	 * can be looked up later
	 */
	fwnode_handle_get(child);
	device_set_node(&phy->mdio.dev, child);

	/* All data is now stored in the phy struct;
	 * register it
	 */
	rc = phy_device_register(phy);
	if (rc) {
		device_set_node(&phy->mdio.dev, NULL);
		fwnode_handle_put(child);
		return rc;
	}

	dev_dbg(&mdio->dev, "registered phy fwnode %pfw at address %i\n",
		child, addr);
	return 0;
}
EXPORT_SYMBOL(fwnode_mdiobus_phy_device_register);

int fwnode_mdiobus_register_phy(struct mii_bus *bus,
				struct fwnode_handle *child, u32 addr)
{
	struct mii_timestamper *mii_ts = NULL;
	struct pse_control *psec = NULL;
	struct phy_device *phy;
	bool is_c45;
	u32 phy_id;
	int rc;

	psec = fwnode_find_pse_control(child);
	if (IS_ERR(psec))
		return PTR_ERR(psec);

	mii_ts = fwnode_find_mii_timestamper(child);
	if (IS_ERR(mii_ts)) {
		rc = PTR_ERR(mii_ts);
		goto clean_pse;
	}

	is_c45 = fwnode_device_is_compatible(child, "ethernet-phy-ieee802.3-c45");
	if (is_c45 || fwnode_get_phy_id(child, &phy_id))
		phy = get_phy_device(bus, addr, is_c45);
	else
		phy = phy_device_create(bus, addr, phy_id, 0, NULL);
	if (IS_ERR(phy)) {
		rc = PTR_ERR(phy);
		goto clean_mii_ts;
	}

	if (is_acpi_node(child)) {
		phy->irq = bus->irq[addr];

		/* Associate the fwnode with the device structure so it
		 * can be looked up later.
		 */
		phy->mdio.dev.fwnode = fwnode_handle_get(child);

		/* All data is now stored in the phy struct, so register it */
		rc = phy_device_register(phy);
		if (rc) {
			phy->mdio.dev.fwnode = NULL;
			fwnode_handle_put(child);
			goto clean_phy;
		}
	} else if (is_of_node(child)) {
		rc = fwnode_mdiobus_phy_device_register(bus, phy, child, addr);
		if (rc)
			goto clean_phy;
	}

	phy->psec = psec;

	/* phy->mii_ts may already be defined by the PHY driver. A
	 * mii_timestamper probed via the device tree will still have
	 * precedence.
	 */
	if (mii_ts)
		phy->mii_ts = mii_ts;

	return 0;

clean_phy:
	phy_device_free(phy);
clean_mii_ts:
	unregister_mii_timestamper(mii_ts);
clean_pse:
	pse_control_put(psec);

	return rc;
}
EXPORT_SYMBOL(fwnode_mdiobus_register_phy);

bool fwnode_mdiobus_child_is_phy(struct fwnode_handle *child)
{
	u32 phy_id;

	if (fwnode_get_phy_id(child, &phy_id) != -EINVAL)
		return true;

	if (fwnode_property_match_string(child, "compatible",
					 "ethernet-phy-ieee802.3-c45") >= 0)
		return true;

	if (fwnode_property_match_string(child, "compatible",
					 "ethernet-phy-ieee802.3-c22") >= 0)
		return true;

	if (!fwnode_property_present(child, "compatible"))
		return true;

	return false;
}
EXPORT_SYMBOL(fwnode_mdiobus_child_is_phy);

int fwnode_mdiobus_register_device(struct mii_bus *bus,
				   struct fwnode_handle *child, u32 addr)
{
	struct mdio_device *mdiodev;
	int rc;

	mdiodev = mdio_device_create(bus, addr);
	if (IS_ERR(mdiodev))
		return PTR_ERR(mdiodev);

	fwnode_handle_get(child);
	device_set_node(&mdiodev->dev, child);

	/* All data is now stored in the mdiodev struct; register it. */
	rc = mdio_device_register(mdiodev);
	if (rc) {
		device_set_node(&mdiodev->dev, NULL);
		fwnode_handle_put(child);
		mdio_device_free(mdiodev);
		return rc;
	}

	dev_err(&mdiodev->dev, "registered mdio device %p fwnode at address %i\n",
		child, addr);
	return 0;
}
EXPORT_SYMBOL(fwnode_mdiobus_register_device);
