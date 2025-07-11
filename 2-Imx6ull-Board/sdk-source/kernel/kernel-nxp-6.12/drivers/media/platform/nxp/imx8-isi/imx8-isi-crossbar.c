// SPDX-License-Identifier: GPL-2.0-only
/*
 * i.MX8 ISI - Input crossbar switch
 *
 * Copyright (c) 2022 Laurent Pinchart <laurent.pinchart@ideasonboard.com>
 */

#include <linux/device.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/minmax.h>
#include <linux/of_graph.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/types.h>

#include <media/media-entity.h>
#include <media/v4l2-subdev.h>

#include "imx8-isi-core.h"

static inline struct mxc_isi_crossbar *to_isi_crossbar(struct v4l2_subdev *sd)
{
	return container_of(sd, struct mxc_isi_crossbar, sd);
}

static int mxc_isi_crossbar_gasket_enable(struct mxc_isi_crossbar *xbar,
					  struct v4l2_subdev_state *state,
					  struct v4l2_subdev *remote_sd,
					  u32 remote_pad, unsigned int port)
{
	struct mxc_isi_dev *isi = xbar->isi;
	const struct mxc_gasket_ops *gasket_ops = isi->pdata->gasket_ops;
	const struct v4l2_mbus_framefmt *fmt;
	struct v4l2_mbus_frame_desc fd;
	unsigned int stream;
	int ret;

	if (!gasket_ops)
		return 0;

	/*
	 * Configure and enable the gasket with the frame size and CSI-2 data
	 * type. For YUV422 8-bit, enable dual component mode unconditionally,
	 * to match the configuration of the CSIS.
	 */

	ret = v4l2_subdev_call(remote_sd, pad, get_frame_desc, remote_pad, &fd);
	if (ret) {
		dev_err(isi->dev,
			"failed to get frame descriptor from '%s':%u: %d\n",
			remote_sd->name, remote_pad, ret);
		return ret;
	}

	/*
	 * For single or multiple stream, only the first stream be used
	 * since gasket enable callback be called only once.
	 */
	stream = fd.num_entries > 0 ? fd.entry[0].stream : 0;
	fmt = v4l2_subdev_state_get_format(state, port, stream);
	if (!fmt)
		return -EINVAL;

	gasket_ops->enable(isi, &fd, fmt, port);
	return 0;
}

static void mxc_isi_crossbar_gasket_disable(struct mxc_isi_crossbar *xbar,
					    unsigned int port)
{
	struct mxc_isi_dev *isi = xbar->isi;
	const struct mxc_gasket_ops *gasket_ops = isi->pdata->gasket_ops;

	if (!gasket_ops)
		return;

	gasket_ops->disable(isi, port);
}

/* -----------------------------------------------------------------------------
 * V4L2 subdev operations
 */

static const struct v4l2_mbus_framefmt mxc_isi_crossbar_default_format = {
	.code = MXC_ISI_DEF_MBUS_CODE_SINK,
	.width = MXC_ISI_DEF_WIDTH,
	.height = MXC_ISI_DEF_HEIGHT,
	.field = V4L2_FIELD_NONE,
	.colorspace = MXC_ISI_DEF_COLOR_SPACE,
	.ycbcr_enc = MXC_ISI_DEF_YCBCR_ENC,
	.quantization = MXC_ISI_DEF_QUANTIZATION,
	.xfer_func = MXC_ISI_DEF_XFER_FUNC,
};

static int __mxc_isi_crossbar_set_routing(struct v4l2_subdev *sd,
					  struct v4l2_subdev_state *state,
					  struct v4l2_subdev_krouting *routing)
{
	struct mxc_isi_crossbar *xbar = to_isi_crossbar(sd);
	struct v4l2_subdev_route *route;
	int ret;

	ret = v4l2_subdev_routing_validate(sd, routing,
					   V4L2_SUBDEV_ROUTING_NO_N_TO_1);
	if (ret)
		return ret;

	/* The memory input can be routed to the first pipeline only. */
	for_each_active_route(&state->routing, route) {
		if (route->sink_pad == xbar->num_sinks - 1 &&
		    route->source_pad != xbar->num_sinks) {
			dev_dbg(xbar->isi->dev,
				"invalid route from memory input (%u) to pipe %u\n",
				route->sink_pad,
				route->source_pad - xbar->num_sinks);
			return -EINVAL;
		}
	}

	return v4l2_subdev_set_routing_with_fmt(sd, state, routing,
						&mxc_isi_crossbar_default_format);
}

static struct v4l2_subdev *
mxc_isi_crossbar_xlate_streams(struct mxc_isi_crossbar *xbar,
			       struct v4l2_subdev_state *state,
			       u32 source_pad, u64 source_streams,
			       u32 *__sink_pad, u64 *__sink_streams,
			       u32 *remote_pad)
{
	struct v4l2_subdev_route *route;
	struct v4l2_subdev *sd;
	struct media_pad *pad;
	u64 sink_streams = 0;
	int sink_pad = -1;

	/*
	 * Translate the source pad and streams to the sink side. The routing
	 * validation forbids stream merging, so all matching entries in the
	 * routing table are guaranteed to have the same sink pad.
	 *
	 * TODO: This is likely worth a helper function, it could perhaps be
	 * supported by v4l2_subdev_state_xlate_streams() with pad1 set to -1.
	 */
	for_each_active_route(&state->routing, route) {
		if (route->source_pad != source_pad ||
		    !(source_streams & BIT(route->source_stream)))
			continue;

		sink_streams |= BIT(route->sink_stream);
		sink_pad = route->sink_pad;
	}

	if (sink_pad < 0) {
		dev_dbg(xbar->isi->dev,
			"no stream connected to pipeline %u\n",
			source_pad - xbar->num_sinks);
		return ERR_PTR(-EPIPE);
	}

	pad = media_pad_remote_pad_first(&xbar->pads[sink_pad]);
	if (!pad) {
		dev_err(xbar->isi->dev,
			"no remote pad found for sink pad %u\n",
			sink_pad);
		return ERR_PTR(-EPIPE);
	}

	sd = media_entity_to_v4l2_subdev(pad->entity);
	if (!sd) {
		dev_dbg(xbar->isi->dev,
			"no entity connected to crossbar input %u\n",
			sink_pad);
		return ERR_PTR(-EPIPE);
	}

	*__sink_pad = sink_pad;
	*__sink_streams = sink_streams;
	*remote_pad = pad->index;

	return sd;
}

static int mxc_isi_create_default_routing(struct mxc_isi_crossbar *xbar,
					   struct v4l2_subdev_route *routes)
{
	struct device *dev = xbar->isi->dev;
	struct device_node *node;
	unsigned int index = 0;
	int i, j;

	for_each_endpoint_of_node(dev->of_node, node) {
		struct of_endpoint ep;

		of_graph_parse_endpoint(node, &ep);

		if (ep.port > xbar->isi->pdata->num_ports) {
			dev_err(dev, "Invalid port number(%d)\n", ep.port);
			return -EINVAL;
		}

		xbar->inputs[ep.port].connected = true;
	}

	for (i = 0; i < xbar->num_sources; ++i) {
		struct v4l2_subdev_route *route = &routes[i];

		j = index;
		while (j < xbar->num_sinks) {
			if (!xbar->inputs[j].connected) {
				j = (++index) % xbar->num_sinks;
				continue;
			}

			route->sink_pad = j;
			route->source_pad = i + xbar->num_sinks;
			route->flags = V4L2_SUBDEV_ROUTE_FL_ACTIVE;

			index = (j + 1) % xbar->num_sinks;
			dev_dbg(dev, "route: sink(%d) -> source(%d)\n",
				route->sink_pad, route->source_pad);
			break;
		}
	}

	return 0;
}

static int mxc_isi_crossbar_init_state(struct v4l2_subdev *sd,
				       struct v4l2_subdev_state *state)
{
	struct mxc_isi_crossbar *xbar = to_isi_crossbar(sd);
	struct v4l2_subdev_krouting routing = { };
	struct v4l2_subdev_route *routes;
	int ret;

	/*
	 * Create a 1:N mapping between pixel link inputs and outputs to
	 * pipelines by according to the number of pixel links inputs and
	 * ISI channels. The algorithm will divide the ISI channel equally
	 * to each pixel link input which connect to a remote device.
	 */
	routes = kcalloc(xbar->num_sources, sizeof(*routes), GFP_KERNEL);
	if (!routes)
		return -ENOMEM;

	ret = mxc_isi_create_default_routing(xbar, routes);
	if (ret < 0) {
		kfree(routes);
		return ret;
	}

	routing.num_routes = xbar->num_sources;
	routing.routes = routes;

	ret = __mxc_isi_crossbar_set_routing(sd, state, &routing);

	kfree(routes);

	return ret;
}

static int mxc_isi_crossbar_enum_mbus_code(struct v4l2_subdev *sd,
					   struct v4l2_subdev_state *state,
					   struct v4l2_subdev_mbus_code_enum *code)
{
	struct mxc_isi_crossbar *xbar = to_isi_crossbar(sd);
	const struct mxc_isi_bus_format_info *info;

	if (code->pad >= xbar->num_sinks) {
		const struct v4l2_mbus_framefmt *format;

		/*
		 * The media bus code on source pads is identical to the
		 * connected sink pad.
		 */
		if (code->index > 0)
			return -EINVAL;

		format = v4l2_subdev_state_get_opposite_stream_format(state,
								      code->pad,
								      code->stream);
		if (!format)
			return -EINVAL;

		code->code = format->code;

		return 0;
	}

	info = mxc_isi_bus_format_by_index(code->index, MXC_ISI_PIPE_PAD_SINK);
	if (!info)
		return -EINVAL;

	code->code = info->mbus_code;

	return 0;
}

static int mxc_isi_crossbar_set_fmt(struct v4l2_subdev *sd,
				    struct v4l2_subdev_state *state,
				    struct v4l2_subdev_format *fmt)
{
	struct mxc_isi_crossbar *xbar = to_isi_crossbar(sd);
	struct v4l2_mbus_framefmt *sink_fmt;
	struct v4l2_subdev_route *route;

	if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE &&
	    media_pad_is_streaming(&xbar->pads[fmt->pad]))
		return -EBUSY;

	/*
	 * The source pad format is always identical to the sink pad format and
	 * can't be modified.
	 */
	if (fmt->pad >= xbar->num_sinks)
		return v4l2_subdev_get_fmt(sd, state, fmt);

	/* Validate the requested format. */
	if (!mxc_isi_bus_format_by_code(fmt->format.code, MXC_ISI_PIPE_PAD_SINK))
		fmt->format.code = MXC_ISI_DEF_MBUS_CODE_SINK;

	fmt->format.width = clamp_t(unsigned int, fmt->format.width,
				    MXC_ISI_MIN_WIDTH, MXC_ISI_MAX_WIDTH_CHAINED);
	fmt->format.height = clamp_t(unsigned int, fmt->format.height,
				     MXC_ISI_MIN_HEIGHT, MXC_ISI_MAX_HEIGHT);
	fmt->format.field = V4L2_FIELD_NONE;

	/*
	 * Set the format on the sink stream and propagate it to the source
	 * streams.
	 */
	sink_fmt = v4l2_subdev_state_get_format(state, fmt->pad, fmt->stream);
	if (!sink_fmt)
		return -EINVAL;

	*sink_fmt = fmt->format;

	/* TODO: A format propagation helper would be useful. */
	for_each_active_route(&state->routing, route) {
		struct v4l2_mbus_framefmt *source_fmt;

		if (route->sink_pad != fmt->pad ||
		    route->sink_stream != fmt->stream)
			continue;

		source_fmt = v4l2_subdev_state_get_format(state,
							  route->source_pad,
							  route->source_stream);
		if (!source_fmt)
			return -EINVAL;

		*source_fmt = fmt->format;
	}

	return 0;
}

static int mxc_isi_get_frame_desc(struct v4l2_subdev *sd, unsigned int pad,
				  struct v4l2_mbus_frame_desc *fd)
{
	struct mxc_isi_crossbar *xbar = to_isi_crossbar(sd);
	struct device *dev = xbar->isi->dev;
	struct v4l2_subdev_route *route;
	struct v4l2_subdev_state *state;
	int ret;

	if (pad < xbar->num_sinks)
		return -EINVAL;

	memset(fd, 0, sizeof(*fd));

	fd->type = V4L2_MBUS_FRAME_DESC_TYPE_CSI2;

	state = v4l2_subdev_lock_and_get_active_state(sd);

	for_each_active_route(&state->routing, route) {
		struct v4l2_mbus_frame_desc_entry *source_entry = NULL;
		struct v4l2_mbus_frame_desc source_fd;
		struct v4l2_subdev *remote_sd = NULL;
		struct media_pad *remote_pad;
		unsigned int i;

		if (route->source_pad != pad)
			continue;

		remote_pad = media_pad_remote_pad_first(&xbar->pads[route->sink_pad]);
		if (remote_pad)
			remote_sd = media_entity_to_v4l2_subdev(remote_pad->entity);
		if (!remote_sd) {
			dev_err(dev, "no entity connected to crossbar input %u\n",
				route->sink_pad);
			ret = -EPIPE;
			goto out_unlock;
		}

		ret = v4l2_subdev_call(remote_sd, pad, get_frame_desc,
				       remote_pad->index, &source_fd);
		if (ret < 0) {
			dev_err(dev, "Failed to get source frame desc from pad %u\n",
				route->sink_pad);
			goto out_unlock;
		}

		for (i = 0; i < source_fd.num_entries; i++) {
			if (source_fd.entry[i].stream == route->sink_stream) {
				source_entry = &source_fd.entry[i];
				break;
			}
		}

		if (!source_entry) {
			dev_err(dev, "Failed to find stream from source frames desc, pad %u\n",
				route->sink_pad);
			ret = -EPIPE;
			goto out_unlock;
		}

		fd->entry[fd->num_entries].stream = route->source_stream;
		fd->entry[fd->num_entries].flags = source_entry->flags;
		fd->entry[fd->num_entries].length = source_entry->length;
		fd->entry[fd->num_entries].pixelcode = source_entry->pixelcode;
		fd->entry[fd->num_entries].bus.csi2.vc = source_entry->bus.csi2.vc;
		fd->entry[fd->num_entries].bus.csi2.dt = source_entry->bus.csi2.dt;

		fd->num_entries++;
	}

out_unlock:
	v4l2_subdev_unlock_state(state);
	return ret;
}

static int mxc_isi_crossbar_set_routing(struct v4l2_subdev *sd,
					struct v4l2_subdev_state *state,
					enum v4l2_subdev_format_whence which,
					struct v4l2_subdev_krouting *routing)
{
	if (which == V4L2_SUBDEV_FORMAT_ACTIVE &&
	    media_entity_is_streaming(&sd->entity))
		return -EBUSY;

	return __mxc_isi_crossbar_set_routing(sd, state, routing);
}

static int mxc_isi_crossbar_enable_streams(struct v4l2_subdev *sd,
					   struct v4l2_subdev_state *state,
					   u32 pad, u64 streams_mask)
{
	struct mxc_isi_crossbar *xbar = to_isi_crossbar(sd);
	struct v4l2_subdev *remote_sd;
	struct mxc_isi_input *input;
	u64 sink_streams;
	u32 sink_pad;
	u32 remote_pad;
	u8 stream_index;
	int ret;

	remote_sd = mxc_isi_crossbar_xlate_streams(xbar, state, pad, streams_mask,
						   &sink_pad, &sink_streams,
						   &remote_pad);
	if (IS_ERR(remote_sd))
		return PTR_ERR(remote_sd);

	input = &xbar->inputs[sink_pad];

	/*
	 * TODO: Track per-stream enable counts to support multiplexed
	 * streams.
	 */
	if (!input->enabled_streams) {
		ret = mxc_isi_crossbar_gasket_enable(xbar, state, remote_sd,
						     remote_pad, sink_pad);
		if (ret)
			return ret;
	}

	stream_index = clamp_t(u8, ffs(sink_streams), 1, xbar->num_sources);

	/*
	 * For enabled streams, increase the stream counter and return
	 * directly to support ISI stream duplicated feature
	 */
	if (input->enabled_streams & sink_streams) {
		input->enabled_count[(stream_index - 1)]++;
		return 0;
	}

	ret = v4l2_subdev_enable_streams(remote_sd, remote_pad, sink_streams);
	if (ret < 0) {
		if (!input->enabled_streams)
			mxc_isi_crossbar_gasket_disable(xbar, sink_pad);
		return ret;
	}

	input->enabled_streams |= sink_streams;
	input->enabled_count[(stream_index - 1)]++;

	return 0;
}

static int mxc_isi_crossbar_disable_streams(struct v4l2_subdev *sd,
					    struct v4l2_subdev_state *state,
					    u32 pad, u64 streams_mask)
{
	struct mxc_isi_crossbar *xbar = to_isi_crossbar(sd);
	struct v4l2_subdev *remote_sd;
	struct mxc_isi_input *input;
	u64 sink_streams;
	u32 sink_pad;
	u32 remote_pad;
	u8 stream_index;
	int ret = 0;

	remote_sd = mxc_isi_crossbar_xlate_streams(xbar, state, pad, streams_mask,
						   &sink_pad, &sink_streams,
						   &remote_pad);
	if (IS_ERR(remote_sd))
		return PTR_ERR(remote_sd);

	input = &xbar->inputs[sink_pad];
	stream_index = clamp_t(u8, ffs(sink_streams), 1, xbar->num_sources);

	/*
	 * For disabled streams or stream which counter is non-zero,
	 * decrease the stream counter and return directly.
	 */
	if (!(input->enabled_streams & sink_streams) ||
	    --input->enabled_count[(stream_index - 1)])
		return 0;

	ret = v4l2_subdev_disable_streams(remote_sd, remote_pad, sink_streams);
	if (ret)
		dev_err(xbar->isi->dev,
			"failed to %s streams 0x%llx on '%s':%u: %d\n",
			"disable", sink_streams, remote_sd->name,
			remote_pad, ret);

	input->enabled_streams &= ~sink_streams;

	if (!input->enabled_streams)
		mxc_isi_crossbar_gasket_disable(xbar, sink_pad);

	return ret;
}

static const struct v4l2_subdev_pad_ops mxc_isi_crossbar_subdev_pad_ops = {
	.enum_mbus_code = mxc_isi_crossbar_enum_mbus_code,
	.get_fmt = v4l2_subdev_get_fmt,
	.set_fmt = mxc_isi_crossbar_set_fmt,
	.get_frame_desc = mxc_isi_get_frame_desc,
	.set_routing = mxc_isi_crossbar_set_routing,
	.enable_streams = mxc_isi_crossbar_enable_streams,
	.disable_streams = mxc_isi_crossbar_disable_streams,
};

static const struct v4l2_subdev_ops mxc_isi_crossbar_subdev_ops = {
	.pad = &mxc_isi_crossbar_subdev_pad_ops,
};

static const struct v4l2_subdev_internal_ops mxc_isi_crossbar_internal_ops = {
	.init_state = mxc_isi_crossbar_init_state,
};

static const struct media_entity_operations mxc_isi_cross_entity_ops = {
	.get_fwnode_pad = v4l2_subdev_get_fwnode_pad_1_to_1,
	.link_validate	= v4l2_subdev_link_validate,
	.has_pad_interdep = v4l2_subdev_has_pad_interdep,
};

/* -----------------------------------------------------------------------------
 * Init & cleanup
 */

static int mxc_isi_input_stream_alloc(struct mxc_isi_crossbar *xbar)
{
	unsigned int i;

	/*
	 * Track per-stream enable counts to support multiplexed streams
	 */
	for (i = 0; i < xbar->num_sinks; ++i) {
		struct mxc_isi_input *input = &xbar->inputs[i];

		input->enabled_count = kcalloc(xbar->num_sources,
					       sizeof(*input->enabled_count),
					       GFP_KERNEL);
		if (!input->enabled_count) {
			dev_err(xbar->isi->dev,
				"failed to alloc memory for ISI input(%d)\n", i);
			return -ENOMEM;
		}
	}

	return 0;
}

static void mxc_isi_input_stream_free(struct mxc_isi_crossbar *xbar)
{
	unsigned int i;

	for (i = 0; i < xbar->num_sinks; ++i)
		kfree(xbar->inputs[i].enabled_count);
}

int mxc_isi_crossbar_init(struct mxc_isi_dev *isi)
{
	struct mxc_isi_crossbar *xbar = &isi->crossbar;
	struct v4l2_subdev *sd = &xbar->sd;
	unsigned int num_pads;
	unsigned int i;
	int ret;

	xbar->isi = isi;

	v4l2_subdev_init(sd, &mxc_isi_crossbar_subdev_ops);
	sd->internal_ops = &mxc_isi_crossbar_internal_ops;
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE | V4L2_SUBDEV_FL_STREAMS;
	strscpy(sd->name, "crossbar", sizeof(sd->name));
	sd->dev = isi->dev;

	sd->entity.function = MEDIA_ENT_F_VID_MUX;
	sd->entity.ops = &mxc_isi_cross_entity_ops;

	/*
	 * The subdev has one sink and one source per port, plus one sink for
	 * the memory input.
	 */
	xbar->num_sinks = isi->pdata->num_ports + 1;
	xbar->num_sources = isi->pdata->num_channels;
	num_pads = xbar->num_sinks + xbar->num_sources;

	xbar->pads = kcalloc(num_pads, sizeof(*xbar->pads), GFP_KERNEL);
	if (!xbar->pads)
		return -ENOMEM;

	xbar->inputs = kcalloc(xbar->num_sinks, sizeof(*xbar->inputs),
			       GFP_KERNEL);
	if (!xbar->inputs) {
		ret = -ENOMEM;
		goto err_free;
	}

	ret = mxc_isi_input_stream_alloc(xbar);
	if (ret)
		goto err_free;

	for (i = 0; i < xbar->num_sinks; ++i)
		xbar->pads[i].flags = MEDIA_PAD_FL_SINK
				    | MEDIA_PAD_FL_MUST_CONNECT;
	for (i = 0; i < xbar->num_sources; ++i)
		xbar->pads[i + xbar->num_sinks].flags = MEDIA_PAD_FL_SOURCE;

	ret = media_entity_pads_init(&sd->entity, num_pads, xbar->pads);
	if (ret)
		goto err_free_cnt;

	ret = v4l2_subdev_init_finalize(sd);
	if (ret < 0)
		goto err_entity;

	return 0;

err_entity:
	media_entity_cleanup(&sd->entity);
err_free_cnt:
	mxc_isi_input_stream_free(xbar);
err_free:
	kfree(xbar->pads);
	kfree(xbar->inputs);

	return ret;
}

void mxc_isi_crossbar_cleanup(struct mxc_isi_crossbar *xbar)
{
	mxc_isi_input_stream_free(xbar);
	media_entity_cleanup(&xbar->sd.entity);
	kfree(xbar->pads);
	kfree(xbar->inputs);
}

int mxc_isi_crossbar_register(struct mxc_isi_crossbar *xbar)
{
	return v4l2_device_register_subdev(&xbar->isi->v4l2_dev, &xbar->sd);
}

void mxc_isi_crossbar_unregister(struct mxc_isi_crossbar *xbar)
{
}
