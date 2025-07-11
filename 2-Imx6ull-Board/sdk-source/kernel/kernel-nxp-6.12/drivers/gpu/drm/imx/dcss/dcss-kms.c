// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2019 NXP.
 */

#include <drm/bridge/cdns-mhdp.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_blend.h>
#include <drm/drm_bridge_connector.h>
#include <drm/drm_drv.h>
#include <drm/drm_fbdev_dma.h>
#include <drm/drm_gem_dma_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_of.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_vblank.h>
#include <linux/component.h>

#include "dcss-dev.h"
#include "dcss-kms.h"

DEFINE_DRM_GEM_DMA_FOPS(dcss_cma_fops);

static int dcss_kms_atomic_check(struct drm_device *dev,
				 struct drm_atomic_state *state)
{
	int ret;

	ret = drm_atomic_helper_check_modeset(dev, state);
	if (ret)
		return ret;

	ret = drm_atomic_normalize_zpos(dev, state);
	if (ret)
		return ret;

	ret = dcss_crtc_setup_opipe(dev, state);
	if (ret)
		return ret;

	return drm_atomic_helper_check_planes(dev, state);
}

static const struct drm_mode_config_funcs dcss_drm_mode_config_funcs = {
	.fb_create = drm_gem_fb_create,
	.atomic_check = dcss_kms_atomic_check,
	.atomic_commit = drm_atomic_helper_commit,
};

static const struct drm_driver dcss_kms_driver = {
	.driver_features	= DRIVER_MODESET | DRIVER_GEM | DRIVER_ATOMIC,
	DRM_GEM_DMA_DRIVER_OPS,
	.gem_prime_import	= drm_gem_prime_import,
	.fops			= &dcss_cma_fops,
	.name			= "imx-dcss",
	.desc			= "i.MX8MQ Display Subsystem",
	.date			= "20190917",
	.major			= 1,
	.minor			= 0,
	.patchlevel		= 0,
};

static const struct drm_mode_config_helper_funcs dcss_mode_config_helpers = {
	.atomic_commit_tail = drm_atomic_helper_commit_tail_rpm,
};

static void dcss_kms_mode_config_init(struct dcss_kms_dev *kms)
{
	struct drm_mode_config *config = &kms->base.mode_config;

	drm_mode_config_init(&kms->base);

	config->min_width = 1;
	config->min_height = 1;
	config->max_width = 4096;
	config->max_height = 4096;
	config->normalize_zpos = true;

	config->funcs = &dcss_drm_mode_config_funcs;
	config->helper_private = &dcss_mode_config_helpers;
}

static const struct drm_encoder_funcs dcss_kms_simple_encoder_funcs = {
	.destroy = drm_encoder_cleanup,
};

static int dcss_kms_bridge_connector_init(struct dcss_kms_dev *kms)
{
	struct drm_device *ddev = &kms->base;
	struct drm_encoder *encoder = &kms->encoder;
	struct drm_crtc *crtc = (struct drm_crtc *)&kms->crtc;
	struct drm_panel *panel;
	struct drm_bridge *bridge;
	int ret;

	ret = drm_of_find_panel_or_bridge(ddev->dev->of_node, 0, 0,
					  &panel, &bridge);
	if (ret)
		return ret;

	if (!bridge) {
		dev_err(ddev->dev, "No bridge found %d.\n", ret);
		return -ENODEV;
	}

	encoder->possible_crtcs = drm_crtc_mask(crtc);

	ret = drm_encoder_init(&kms->base, encoder,
			       &dcss_kms_simple_encoder_funcs,
			       DRM_MODE_ENCODER_NONE, NULL);
	if (ret) {
		dev_err(ddev->dev, "Failed initializing encoder %d.\n", ret);
		return ret;
	}

	ret = drm_bridge_attach(encoder, bridge, NULL,
				DRM_BRIDGE_ATTACH_NO_CONNECTOR);
	if (ret < 0)
		return ret;

	kms->connector = drm_bridge_connector_init(ddev, encoder);
	if (IS_ERR(kms->connector)) {
		dev_err(ddev->dev, "Unable to create bridge connector.\n");
		return PTR_ERR(kms->connector);
	}

	drm_connector_attach_encoder(kms->connector, encoder);

	return 0;
}

struct dcss_kms_dev *dcss_kms_attach(struct dcss_dev *dcss, bool componentized)
{
	struct dcss_kms_dev *kms;
	struct drm_device *drm;
	struct dcss_crtc *crtc;
	int ret;

	kms = devm_drm_dev_alloc(dcss->dev, &dcss_kms_driver,
				 struct dcss_kms_dev, base);
	if (IS_ERR(kms))
		return kms;

	drm = &kms->base;
	crtc = &kms->crtc;

	drm->dev_private = dcss;

	dcss_kms_mode_config_init(kms);

	ret = drm_vblank_init(drm, 1);
	if (ret)
		goto cleanup_mode_config;

	if (!componentized) {
		ret = dcss_kms_bridge_connector_init(kms);
		if (ret)
			goto cleanup_mode_config;
	}

	ret = dcss_crtc_init(crtc, drm);
	if (ret)
		goto cleanup_mode_config;

	if (componentized) {
		ret = component_bind_all(dcss->dev, kms);
		if (ret)
			goto cleanup_crtc;
	}

	drm_mode_config_reset(drm);

	dcss_crtc_attach_color_mgmt_properties(crtc);

	drm_kms_helper_poll_init(drm);

	ret = drm_dev_register(drm, 0);
	if (ret)
		goto cleanup_crtc;

	drm_fbdev_dma_setup(drm, 32);

	return kms;

cleanup_crtc:
	drm_kms_helper_poll_fini(drm);
	dcss_crtc_deinit(crtc, drm);

cleanup_mode_config:
	drm_mode_config_cleanup(drm);
	drm->dev_private = NULL;

	return ERR_PTR(ret);
}

void dcss_kms_detach(struct dcss_kms_dev *kms, bool componentized)
{
	struct drm_device *drm = &kms->base;
	struct dcss_dev *dcss = drm->dev_private;

	drm_dev_unregister(drm);
	drm_kms_helper_poll_fini(drm);
	drm_atomic_helper_shutdown(drm);
	drm_crtc_vblank_off(&kms->crtc.base);
	drm_mode_config_cleanup(drm);
	dcss_crtc_deinit(&kms->crtc, drm);
	if (componentized)
		component_unbind_all(dcss->dev, drm);
	drm->dev_private = NULL;
}

void dcss_kms_shutdown(struct dcss_kms_dev *kms)
{
	struct drm_device *drm = &kms->base;

	drm_atomic_helper_shutdown(drm);
}
