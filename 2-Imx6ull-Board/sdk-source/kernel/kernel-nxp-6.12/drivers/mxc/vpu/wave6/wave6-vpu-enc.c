// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Wave6 series multi-standard codec IP - encoder interface
 *
 * Copyright (C) 2021 CHIPS&MEDIA INC
 */
#include <linux/pm_runtime.h>
#include <linux/swiotlb.h>
#include "wave6-vpu.h"
#include "wave6-vpu-dbg.h"

#define VPU_ENC_DEV_NAME "C&M Wave6 VPU encoder"
#define VPU_ENC_DRV_NAME "wave6-enc"

static const struct vpu_format wave6_vpu_enc_fmt_list[2][23] = {
	[VPU_FMT_TYPE_CODEC] = {
		{
			.v4l2_pix_fmt = V4L2_PIX_FMT_HEVC,
			.max_width = W6_MAX_ENC_PIC_WIDTH,
			.min_width = W6_MIN_ENC_PIC_WIDTH,
			.max_height = W6_MAX_ENC_PIC_HEIGHT,
			.min_height = W6_MIN_ENC_PIC_HEIGHT,
			.num_planes = 1,
		},
		{
			.v4l2_pix_fmt = V4L2_PIX_FMT_H264,
			.max_width = W6_MAX_ENC_PIC_WIDTH,
			.min_width = W6_MIN_ENC_PIC_WIDTH,
			.max_height = W6_MAX_ENC_PIC_HEIGHT,
			.min_height = W6_MIN_ENC_PIC_HEIGHT,
			.num_planes = 1,
		},
	},
	[VPU_FMT_TYPE_RAW] = {
		{
			.v4l2_pix_fmt = V4L2_PIX_FMT_YUV420,
			.max_width = W6_MAX_ENC_PIC_WIDTH,
			.min_width = W6_MIN_ENC_PIC_WIDTH,
			.max_height = W6_MAX_ENC_PIC_HEIGHT,
			.min_height = W6_MIN_ENC_PIC_HEIGHT,
			.num_planes = 1,
		},
		{
			.v4l2_pix_fmt = V4L2_PIX_FMT_NV12,
			.max_width = W6_MAX_ENC_PIC_WIDTH,
			.min_width = W6_MIN_ENC_PIC_WIDTH,
			.max_height = W6_MAX_ENC_PIC_HEIGHT,
			.min_height = W6_MIN_ENC_PIC_HEIGHT,
			.num_planes = 1,
		},
		{
			.v4l2_pix_fmt = V4L2_PIX_FMT_NV21,
			.max_width = W6_MAX_ENC_PIC_WIDTH,
			.min_width = W6_MIN_ENC_PIC_WIDTH,
			.max_height = W6_MAX_ENC_PIC_HEIGHT,
			.min_height = W6_MIN_ENC_PIC_HEIGHT,
			.num_planes = 1,
		},
		{
			.v4l2_pix_fmt = V4L2_PIX_FMT_YUV422P,
			.max_width = W6_MAX_ENC_PIC_WIDTH,
			.min_width = W6_MIN_ENC_PIC_WIDTH,
			.max_height = W6_MAX_ENC_PIC_HEIGHT,
			.min_height = W6_MIN_ENC_PIC_HEIGHT,
			.num_planes = 1,
		},
		{
			.v4l2_pix_fmt = V4L2_PIX_FMT_NV16,
			.max_width = W6_MAX_ENC_PIC_WIDTH,
			.min_width = W6_MIN_ENC_PIC_WIDTH,
			.max_height = W6_MAX_ENC_PIC_HEIGHT,
			.min_height = W6_MIN_ENC_PIC_HEIGHT,
			.num_planes = 1,
		},
		{
			.v4l2_pix_fmt = V4L2_PIX_FMT_NV61,
			.max_width = W6_MAX_ENC_PIC_WIDTH,
			.min_width = W6_MIN_ENC_PIC_WIDTH,
			.max_height = W6_MAX_ENC_PIC_HEIGHT,
			.min_height = W6_MIN_ENC_PIC_HEIGHT,
			.num_planes = 1,
		},
		{
			.v4l2_pix_fmt = V4L2_PIX_FMT_YUYV,
			.max_width = W6_MAX_ENC_PIC_WIDTH,
			.min_width = W6_MIN_ENC_PIC_WIDTH,
			.max_height = W6_MAX_ENC_PIC_HEIGHT,
			.min_height = W6_MIN_ENC_PIC_HEIGHT,
			.num_planes = 1,
		},
		{
			.v4l2_pix_fmt = V4L2_PIX_FMT_YUV24,
			.max_width = W6_MAX_ENC_PIC_WIDTH,
			.min_width = W6_MIN_ENC_PIC_WIDTH,
			.max_height = W6_MAX_ENC_PIC_HEIGHT,
			.min_height = W6_MIN_ENC_PIC_HEIGHT,
			.num_planes = 1,
		},
		{
			.v4l2_pix_fmt = V4L2_PIX_FMT_NV24,
			.max_width = W6_MAX_ENC_PIC_WIDTH,
			.min_width = W6_MIN_ENC_PIC_WIDTH,
			.max_height = W6_MAX_ENC_PIC_HEIGHT,
			.min_height = W6_MIN_ENC_PIC_HEIGHT,
			.num_planes = 1,
		},
		{
			.v4l2_pix_fmt = V4L2_PIX_FMT_NV42,
			.max_width = W6_MAX_ENC_PIC_WIDTH,
			.min_width = W6_MIN_ENC_PIC_WIDTH,
			.max_height = W6_MAX_ENC_PIC_HEIGHT,
			.min_height = W6_MIN_ENC_PIC_HEIGHT,
			.num_planes = 1,
		},
		{
			.v4l2_pix_fmt = V4L2_PIX_FMT_YUV420M,
			.max_width = W6_MAX_ENC_PIC_WIDTH,
			.min_width = W6_MIN_ENC_PIC_WIDTH,
			.max_height = W6_MAX_ENC_PIC_HEIGHT,
			.min_height = W6_MIN_ENC_PIC_HEIGHT,
			.num_planes = 3,
		},
		{
			.v4l2_pix_fmt = V4L2_PIX_FMT_NV12M,
			.max_width = W6_MAX_ENC_PIC_WIDTH,
			.min_width = W6_MIN_ENC_PIC_WIDTH,
			.max_height = W6_MAX_ENC_PIC_HEIGHT,
			.min_height = W6_MIN_ENC_PIC_HEIGHT,
			.num_planes = 2,
		},
		{
			.v4l2_pix_fmt = V4L2_PIX_FMT_NV21M,
			.max_width = W6_MAX_ENC_PIC_WIDTH,
			.min_width = W6_MIN_ENC_PIC_WIDTH,
			.max_height = W6_MAX_ENC_PIC_HEIGHT,
			.min_height = W6_MIN_ENC_PIC_HEIGHT,
			.num_planes = 2,
		},
		{
			.v4l2_pix_fmt = V4L2_PIX_FMT_YUV422M,
			.max_width = W6_MAX_ENC_PIC_WIDTH,
			.min_width = W6_MIN_ENC_PIC_WIDTH,
			.max_height = W6_MAX_ENC_PIC_HEIGHT,
			.min_height = W6_MIN_ENC_PIC_HEIGHT,
			.num_planes = 3,
		},
		{
			.v4l2_pix_fmt = V4L2_PIX_FMT_NV16M,
			.max_width = W6_MAX_ENC_PIC_WIDTH,
			.min_width = W6_MIN_ENC_PIC_WIDTH,
			.max_height = W6_MAX_ENC_PIC_HEIGHT,
			.min_height = W6_MIN_ENC_PIC_HEIGHT,
			.num_planes = 2,
		},
		{
			.v4l2_pix_fmt = V4L2_PIX_FMT_NV61M,
			.max_width = W6_MAX_ENC_PIC_WIDTH,
			.min_width = W6_MIN_ENC_PIC_WIDTH,
			.max_height = W6_MAX_ENC_PIC_HEIGHT,
			.min_height = W6_MIN_ENC_PIC_HEIGHT,
			.num_planes = 2,
		},
		{
			.v4l2_pix_fmt = V4L2_PIX_FMT_RGB24,
			.max_width = W6_MAX_ENC_PIC_WIDTH,
			.min_width = W6_MIN_ENC_PIC_WIDTH,
			.max_height = W6_MAX_ENC_PIC_HEIGHT,
			.min_height = W6_MIN_ENC_PIC_HEIGHT,
			.num_planes = 1,
		},
		{
			.v4l2_pix_fmt = V4L2_PIX_FMT_P010,
			.max_width = W6_MAX_ENC_PIC_WIDTH,
			.min_width = W6_MIN_ENC_PIC_WIDTH,
			.max_height = W6_MAX_ENC_PIC_HEIGHT,
			.min_height = W6_MIN_ENC_PIC_HEIGHT,
			.num_planes = 1,
		},
		{
			.v4l2_pix_fmt = V4L2_PIX_FMT_ARGB32,
			.max_width = W6_MAX_ENC_PIC_WIDTH,
			.min_width = W6_MIN_ENC_PIC_WIDTH,
			.max_height = W6_MAX_ENC_PIC_HEIGHT,
			.min_height = W6_MIN_ENC_PIC_HEIGHT,
			.num_planes = 1,
		},
		{
			.v4l2_pix_fmt = V4L2_PIX_FMT_XRGB32,
			.max_width = W6_MAX_ENC_PIC_WIDTH,
			.min_width = W6_MIN_ENC_PIC_WIDTH,
			.max_height = W6_MAX_ENC_PIC_HEIGHT,
			.min_height = W6_MIN_ENC_PIC_HEIGHT,
			.num_planes = 1,
		},
		{
			.v4l2_pix_fmt = V4L2_PIX_FMT_RGBA32,
			.max_width = W6_MAX_ENC_PIC_WIDTH,
			.min_width = W6_MIN_ENC_PIC_WIDTH,
			.max_height = W6_MAX_ENC_PIC_HEIGHT,
			.min_height = W6_MIN_ENC_PIC_HEIGHT,
			.num_planes = 1,
		},
		{
			.v4l2_pix_fmt = V4L2_PIX_FMT_RGBX32,
			.max_width = W6_MAX_ENC_PIC_WIDTH,
			.min_width = W6_MIN_ENC_PIC_WIDTH,
			.max_height = W6_MAX_ENC_PIC_HEIGHT,
			.min_height = W6_MIN_ENC_PIC_HEIGHT,
			.num_planes = 1,
		},
		{
			.v4l2_pix_fmt = V4L2_PIX_FMT_ARGB2101010,
			.max_width = W6_MAX_ENC_PIC_WIDTH,
			.min_width = W6_MIN_ENC_PIC_WIDTH,
			.max_height = W6_MAX_ENC_PIC_HEIGHT,
			.min_height = W6_MIN_ENC_PIC_HEIGHT,
			.num_planes = 1,
		},
	}
};

static enum wave_std wave6_to_vpu_wavestd(unsigned int v4l2_pix_fmt)
{
	switch (v4l2_pix_fmt) {
	case V4L2_PIX_FMT_H264:
		return W_AVC_ENC;
	case V4L2_PIX_FMT_HEVC:
		return W_HEVC_ENC;
	default:
		return STD_UNKNOWN;
	}
}

static const struct vpu_format *wave6_find_vpu_fmt(unsigned int v4l2_pix_fmt,
						   enum vpu_fmt_type type)
{
	unsigned int index;

	for (index = 0; index < ARRAY_SIZE(wave6_vpu_enc_fmt_list[type]); index++) {
		if (wave6_vpu_enc_fmt_list[type][index].v4l2_pix_fmt == v4l2_pix_fmt)
			return &wave6_vpu_enc_fmt_list[type][index];
	}

	return NULL;
}

static const struct vpu_format *wave6_find_vpu_fmt_by_idx(unsigned int idx,
							  enum vpu_fmt_type type)
{
	if (idx >= ARRAY_SIZE(wave6_vpu_enc_fmt_list[type]))
		return NULL;

	if (!wave6_vpu_enc_fmt_list[type][idx].v4l2_pix_fmt)
		return NULL;

	return &wave6_vpu_enc_fmt_list[type][idx];
}

static u32 wave6_cpb_size_msec(u32 cpb_size_kb, u32 bit_rate)
{
	u64 cpb_size_bit;
	u64 cpb_size_msec;

	cpb_size_bit = (u64)cpb_size_kb * 1000 * BITS_PER_BYTE;
	cpb_size_msec = (cpb_size_bit * 1000) / bit_rate;

	if (cpb_size_msec < 10 || cpb_size_msec > 100000)
		cpb_size_msec = 10000;

	return cpb_size_msec;
}

static void wave6_vpu_enc_release_fb(struct vpu_instance *inst)
{
	int i;

	for (i = 0; i < WAVE6_MAX_FBS; i++) {
		wave6_free_dma(&inst->frame_vbuf[i]);
		memset(&inst->frame_buf[i], 0, sizeof(struct frame_buffer));
		wave6_free_dma(&inst->aux_vbuf[AUX_BUF_FBC_Y_TBL][i]);
		wave6_free_dma(&inst->aux_vbuf[AUX_BUF_FBC_C_TBL][i]);
		wave6_free_dma(&inst->aux_vbuf[AUX_BUF_MV_COL][i]);
		wave6_free_dma(&inst->aux_vbuf[AUX_BUF_SUB_SAMPLE][i]);
		wave6_free_dma(&inst->aux_vbuf[AUX_BUF_DEF_CDF][i]);
	}
}

static void wave6_vpu_enc_destroy_instance(struct vpu_instance *inst)
{
	u32 fail_res;
	int ret;

	dprintk(inst->dev->dev, "[%d] destroy instance\n", inst->id);
	wave6_vpu_remove_dbgfs_file(inst);

	ret = wave6_vpu_enc_close(inst, &fail_res);
	if (ret) {
		dev_err(inst->dev->dev, "failed destroy instance: %d (%d)\n",
			ret, fail_res);
	}

	wave6_vpu_enc_release_fb(inst);
	wave6_free_dma(&inst->ar_vbuf);

	wave6_vpu_set_instance_state(inst, VPU_INST_STATE_NONE);

	if (!pm_runtime_suspended(inst->dev->dev))
		pm_runtime_put_sync(inst->dev->dev);
}

static struct vb2_v4l2_buffer *wave6_get_valid_src_buf(struct vpu_instance *inst)
{
	struct vb2_v4l2_buffer *vb2_v4l2_buf;
	struct v4l2_m2m_buffer *v4l2_m2m_buf;
	struct vpu_buffer *vpu_buf = NULL;

	v4l2_m2m_for_each_src_buf(inst->v4l2_fh.m2m_ctx, v4l2_m2m_buf) {
		vb2_v4l2_buf = &v4l2_m2m_buf->vb;
		vpu_buf = wave6_to_vpu_buf(vb2_v4l2_buf);

		if (!vpu_buf->consumed) {
			dev_dbg(inst->dev->dev, "no consumed src idx : %d\n",
				vb2_v4l2_buf->vb2_buf.index);
			return vb2_v4l2_buf;
		}
	}

	return NULL;
}

static struct vb2_v4l2_buffer *wave6_get_valid_dst_buf(struct vpu_instance *inst)
{
	struct vb2_v4l2_buffer *vb2_v4l2_buf;
	struct v4l2_m2m_buffer *v4l2_m2m_buf;
	struct vpu_buffer *vpu_buf;

	v4l2_m2m_for_each_dst_buf(inst->v4l2_fh.m2m_ctx, v4l2_m2m_buf) {
		vb2_v4l2_buf = &v4l2_m2m_buf->vb;
		vpu_buf = wave6_to_vpu_buf(vb2_v4l2_buf);

		if (!vpu_buf->consumed) {
			dev_dbg(inst->dev->dev, "no consumed dst idx : %d\n",
				vb2_v4l2_buf->vb2_buf.index);
			return vb2_v4l2_buf;
		}
	}

	return NULL;
}

static void wave6_set_csc(struct vpu_instance *inst, struct enc_param *pic_param)
{
	bool is_10bit = false;

	if (!(inst->src_fmt.pixelformat == V4L2_PIX_FMT_RGB24) &&
	    !(inst->src_fmt.pixelformat == V4L2_PIX_FMT_ARGB32) &&
	    !(inst->src_fmt.pixelformat == V4L2_PIX_FMT_XRGB32) &&
	    !(inst->src_fmt.pixelformat == V4L2_PIX_FMT_RGBA32) &&
	    !(inst->src_fmt.pixelformat == V4L2_PIX_FMT_RGBX32) &&
	    !(inst->src_fmt.pixelformat == V4L2_PIX_FMT_ARGB2101010))
		return;

	if (inst->src_fmt.pixelformat == V4L2_PIX_FMT_ARGB2101010)
		is_10bit = true;

	if (inst->src_fmt.pixelformat == V4L2_PIX_FMT_RGBA32 ||
	    inst->src_fmt.pixelformat == V4L2_PIX_FMT_RGBX32)
		pic_param->csc.format_order = 8;

	if (inst->ycbcr_enc == V4L2_YCBCR_ENC_DEFAULT ||
	    inst->ycbcr_enc == V4L2_YCBCR_ENC_601) {
		if (inst->quantization == V4L2_QUANTIZATION_FULL_RANGE) {
			/*
			 * Y  =  0.299(R)    0.587(G)    0.114(B)
			 * Cb = -0.16874(R) -0.33126(G)  0.5(B)
			 * Cr =  0.5(R)     -0.41869(G) -0.08131(B)
			 */
			pic_param->csc.coef_ry = 0x099;
			pic_param->csc.coef_gy = 0x12d;
			pic_param->csc.coef_by = 0x03a;
			pic_param->csc.coef_rcb = 0xffffffaa;
			pic_param->csc.coef_gcb = 0xffffff56;
			pic_param->csc.coef_bcb = 0x100;
			pic_param->csc.coef_rcr = 0x100;
			pic_param->csc.coef_gcr = 0xffffff2a;
			pic_param->csc.coef_bcr = 0xffffffd6;
			pic_param->csc.offset_y = 0x0;
			pic_param->csc.offset_cb = (is_10bit) ? 0x200 : 0x80;
			pic_param->csc.offset_cr = (is_10bit) ? 0x200 : 0x80;
		} else {
			/*
			 * Y  =  0.258(R)   0.504(G)   0.098(B)
			 * Cb = -0.1484(R) -0.2891(G)  0.4375(B)
			 * Cr =  0.4375(R) -0.3672(G) -0.0703(B)
			 */
			pic_param->csc.coef_ry = 0x084;
			pic_param->csc.coef_gy = 0x102;
			pic_param->csc.coef_by = 0x032;
			pic_param->csc.coef_rcb = 0xffffffb4;
			pic_param->csc.coef_gcb = 0xffffff6c;
			pic_param->csc.coef_bcb = 0x0e0;
			pic_param->csc.coef_rcr = 0x0e0;
			pic_param->csc.coef_gcr = 0xffffff44;
			pic_param->csc.coef_bcr = 0xffffffdc;
			pic_param->csc.offset_y = (is_10bit) ? 0x40 : 0x10;
			pic_param->csc.offset_cb = (is_10bit) ? 0x200 : 0x80;
			pic_param->csc.offset_cr = (is_10bit) ? 0x200 : 0x80;
		}
	} else if (inst->ycbcr_enc == V4L2_YCBCR_ENC_709) {
		if (inst->quantization == V4L2_QUANTIZATION_FULL_RANGE) {
			/*
			 * Y  =  0.2126(R)   0.7152(G)   0.0722(B)
			 * Cb = -0.11457(R) -0.38543(G)  0.5(B)
			 * Cr =  0.5(R)     -0.45415(G) -0.04585(B)
			 */
			pic_param->csc.coef_ry = 0x06d;
			pic_param->csc.coef_gy = 0x16e;
			pic_param->csc.coef_by = 0x025;
			pic_param->csc.coef_rcb = 0xffffffc5;
			pic_param->csc.coef_gcb = 0xffffff3b;
			pic_param->csc.coef_bcb = 0x100;
			pic_param->csc.coef_rcr = 0x100;
			pic_param->csc.coef_gcr = 0xffffff17;
			pic_param->csc.coef_bcr = 0xffffffe9;
			pic_param->csc.offset_y = 0x0;
			pic_param->csc.offset_cb = (is_10bit) ? 0x200 : 0x80;
			pic_param->csc.offset_cr = (is_10bit) ? 0x200 : 0x80;
		} else {
			pic_param->csc.coef_ry = 0x05e;
			pic_param->csc.coef_gy = 0x13b;
			pic_param->csc.coef_by = 0x020;
			pic_param->csc.coef_rcb = 0xffffffcc;
			pic_param->csc.coef_gcb = 0xffffff53;
			pic_param->csc.coef_bcb = 0x0e1;
			pic_param->csc.coef_rcr = 0x0e1;
			pic_param->csc.coef_gcr = 0xffffff34;
			pic_param->csc.coef_bcr = 0xffffffeb;
			pic_param->csc.offset_y = (is_10bit) ? 0x40 : 0x10;
			pic_param->csc.offset_cb = (is_10bit) ? 0x200 : 0x80;
			pic_param->csc.offset_cr = (is_10bit) ? 0x200 : 0x80;
		}
	} else if (inst->ycbcr_enc == V4L2_YCBCR_ENC_BT2020) {
		if (inst->quantization == V4L2_QUANTIZATION_FULL_RANGE) {
			/*
			 * Y  =  0.2627(R)   0.678(G)    0.0593(B)
			 * Cb = -0.13963(R) -0.36037(G)  0.5(B)
			 * Cr =  0.5(R)     -0.45979(G) -0.04021(B)
			 */
			pic_param->csc.coef_ry = 0x087;
			pic_param->csc.coef_gy = 0x15b;
			pic_param->csc.coef_by = 0x01e;
			pic_param->csc.coef_rcb = 0xffffffb9;
			pic_param->csc.coef_gcb = 0xffffff47;
			pic_param->csc.coef_bcb = 0x100;
			pic_param->csc.coef_rcr = 0x100;
			pic_param->csc.coef_gcr = 0xffffff15;
			pic_param->csc.coef_bcr = 0xffffffeb;
			pic_param->csc.offset_y = 0x0;
			pic_param->csc.offset_cb = (is_10bit) ? 0x200 : 0x80;
			pic_param->csc.offset_cr = (is_10bit) ? 0x200 : 0x80;
		} else {
			pic_param->csc.coef_ry = 0x074;
			pic_param->csc.coef_gy = 0x12a;
			pic_param->csc.coef_by = 0x01a;
			pic_param->csc.coef_rcb = 0xffffffc1;
			pic_param->csc.coef_gcb = 0xffffff5e;
			pic_param->csc.coef_bcb = 0x0e1;
			pic_param->csc.coef_rcr = 0x0e1;
			pic_param->csc.coef_gcr = 0xffffff31;
			pic_param->csc.coef_bcr = 0xffffffee;
			pic_param->csc.offset_y = (is_10bit) ? 0x40 : 0x10;
			pic_param->csc.offset_cb = (is_10bit) ? 0x200 : 0x80;
			pic_param->csc.offset_cr = (is_10bit) ? 0x200 : 0x80;
		}
	} else if (inst->ycbcr_enc == V4L2_YCBCR_ENC_SMPTE240M) {
		if (inst->quantization == V4L2_QUANTIZATION_FULL_RANGE) {
			/*
			 * Y  =  0.2122(R)  0.7013(G)  0.0865(B)
			 * Cb = -0.1161(R) -0.3839(G)  0.5(B)
			 * Cr =  0.5(R)    -0.4451(G) -0.0549(B)
			 */
			pic_param->csc.coef_ry = 0x06d;
			pic_param->csc.coef_gy = 0x167;
			pic_param->csc.coef_by = 0x02c;
			pic_param->csc.coef_rcb = 0xffffffc5;
			pic_param->csc.coef_gcb = 0xffffff3b;
			pic_param->csc.coef_bcb = 0x100;
			pic_param->csc.coef_rcr = 0x100;
			pic_param->csc.coef_gcr = 0xffffff1c;
			pic_param->csc.coef_bcr = 0xffffffe4;
			pic_param->csc.offset_y = 0x0;
			pic_param->csc.offset_cb = (is_10bit) ? 0x200 : 0x80;
			pic_param->csc.offset_cr = (is_10bit) ? 0x200 : 0x80;
		} else {
			pic_param->csc.coef_ry = 0x05d;
			pic_param->csc.coef_gy = 0x134;
			pic_param->csc.coef_by = 0x026;
			pic_param->csc.coef_rcb = 0xffffffcc;
			pic_param->csc.coef_gcb = 0xffffff53;
			pic_param->csc.coef_bcb = 0x0e1;
			pic_param->csc.coef_rcr = 0x0e1;
			pic_param->csc.coef_gcr = 0xffffff38;
			pic_param->csc.coef_bcr = 0xffffffe7;
			pic_param->csc.offset_y = (is_10bit) ? 0x40 : 0x10;
			pic_param->csc.offset_cb = (is_10bit) ? 0x200 : 0x80;
			pic_param->csc.offset_cr = (is_10bit) ? 0x200 : 0x80;
		}
	} else if (inst->ycbcr_enc == V4L2_YCBCR_ENC_XV601) {
		if (inst->quantization == V4L2_QUANTIZATION_LIM_RANGE) {
			/*
			 * Y  =  0.2558(R)  0.5021(G)  0.0975(B)
			 * Cb = -0.1476(R) -0.2899(G)  0.4375(B)
			 * Cr =  0.4375(R) -0.3664(G) -0.0711(B)
			 */
			pic_param->csc.coef_ry = 0x083;
			pic_param->csc.coef_gy = 0x101;
			pic_param->csc.coef_by = 0x032;
			pic_param->csc.coef_rcb = 0xffffffb4;
			pic_param->csc.coef_gcb = 0xffffff6c;
			pic_param->csc.coef_bcb = 0x0e0;
			pic_param->csc.coef_rcr = 0x0e0;
			pic_param->csc.coef_gcr = 0xffffff44;
			pic_param->csc.coef_bcr = 0xffffffdc;
			pic_param->csc.offset_y = (is_10bit) ? 0x40 : 0x10;
			pic_param->csc.offset_cb = 0x0;
			pic_param->csc.offset_cr = 0x0;
		}
	} else if (inst->ycbcr_enc == V4L2_YCBCR_ENC_XV709) {
		if (inst->quantization == V4L2_QUANTIZATION_LIM_RANGE) {
			/*
			 * Y  =  0.1819(R)  0.6118(G)  0.0618(B)
			 * Cb = -0.1003(R) -0.3372(G)  0.4375(B)
			 * Cr =  0.4375(R) -0.3974(G) -0.0401(B)
			 */
			pic_param->csc.coef_ry = 0x05d;
			pic_param->csc.coef_gy = 0x139;
			pic_param->csc.coef_by = 0x020;
			pic_param->csc.coef_rcb = 0xffffffcd;
			pic_param->csc.coef_gcb = 0xffffff53;
			pic_param->csc.coef_bcb = 0x0e0;
			pic_param->csc.coef_rcr = 0x0e0;
			pic_param->csc.coef_gcr = 0xffffff35;
			pic_param->csc.coef_bcr = 0xffffffeb;
			pic_param->csc.offset_y = (is_10bit) ? 0x40 : 0x10;
			pic_param->csc.offset_cb = 0x0;
			pic_param->csc.offset_cr = 0x0;
		}
	}
}

static int wave6_allocate_aux_buffer(struct vpu_instance *inst,
				     enum aux_buffer_type type,
				     int num)
{
	struct aux_buffer buf[WAVE6_MAX_FBS];
	struct aux_buffer_info buf_info;
	struct enc_aux_buffer_size_info size_info;
	unsigned int size;
	int i, ret;

	memset(buf, 0, sizeof(buf));

	size_info.width = inst->dst_fmt.width;
	size_info.height = inst->dst_fmt.height;
	size_info.type = type;
	size_info.mirror_direction = inst->enc_ctrls.mirror_direction;
	size_info.rotation_angle = inst->enc_ctrls.rot_angle;

	ret = wave6_vpu_enc_get_aux_buffer_size(inst, size_info, &size);
	if (ret) {
		dev_err(inst->dev->dev, "%s: Get size fail\n", __func__);
		return ret;
	}

	for (i = 0; i < num; i++) {
		inst->aux_vbuf[type][i].size = size;
		ret = wave6_alloc_dma(inst->dev->dev, &inst->aux_vbuf[type][i]);
		if (ret) {
			dev_err(inst->dev->dev, "%s: Alloc fail\n", __func__);
			return ret;
		}

		buf[i].index = i;
		buf[i].addr = inst->aux_vbuf[type][i].daddr;
		buf[i].size = inst->aux_vbuf[type][i].size;
	}

	buf_info.type = type;
	buf_info.num = num;
	buf_info.buf_array = buf;

	ret = wave6_vpu_enc_register_aux_buffer(inst, buf_info);
	if (ret) {
		dev_err(inst->dev->dev, "%s: Register fail\n", __func__);
		return ret;
	}

	return 0;
}

static void wave6_update_frame_buf_addr(struct vpu_instance *inst,
					struct frame_buffer *frame_buf)
{
	const struct v4l2_format_info *fmt_info;
	u32 stride = inst->src_fmt.plane_fmt[0].bytesperline;
	u32 offset;

	fmt_info = v4l2_format_info(inst->src_fmt.pixelformat);
	if (!fmt_info)
		return;

	offset = inst->crop.top * stride + inst->crop.left * fmt_info->bpp[0];
	frame_buf->buf_y += offset;

	stride = DIV_ROUND_UP(stride, fmt_info->bpp[0]) * fmt_info->bpp[1];
	offset = inst->crop.top * stride / fmt_info->vdiv / fmt_info->hdiv
			+ inst->crop.left * fmt_info->bpp[1] / fmt_info->hdiv;
	frame_buf->buf_cb += offset;
	frame_buf->buf_cr += offset;
}

static void wave6_enc_update_seq_param(struct vpu_instance *inst)
{
	struct enc_info *p_enc_info = &inst->codec_info->enc_info;
	struct enc_change_param *p_change_param = &p_enc_info->change_param;
	struct enc_wave_param *p_param = &p_enc_info->open_param.wave_param;

	memset(p_change_param, 0, sizeof(struct enc_change_param));

	/*do we need check bitrate is out of maxmbs*/
	if (p_param->enc_bit_rate != inst->enc_ctrls.bitrate) {
		p_change_param->enable |= ENABLE_SET_RC_TARGET_RATE;
		p_change_param->enc_bit_rate = inst->enc_ctrls.bitrate;
		p_param->enc_bit_rate = inst->enc_ctrls.bitrate;
	}
}

static int wave6_vpu_enc_start_encode(struct vpu_instance *inst)
{
	int ret = -EINVAL;
	struct vb2_v4l2_buffer *src_buf = NULL;
	struct vb2_v4l2_buffer *dst_buf = NULL;
	struct vpu_buffer *src_vbuf = NULL;
	struct vpu_buffer *dst_vbuf = NULL;
	struct frame_buffer frame_buf;
	struct enc_param pic_param;
	struct enc_initial_info initial_info;
	struct enc_info *p_enc_info = &inst->codec_info->enc_info;
	u32 stride = inst->src_fmt.plane_fmt[0].bytesperline;
	u32 luma_size = (stride * inst->src_fmt.height);
	u32 chroma_size;
	u32 fail_res;

	wave6_enc_update_seq_param(inst);
	if (p_enc_info->change_param.enable) {
		pr_info("start encode:dynamic change param\n");
		wave6_vpu_enc_update_seq(inst);
		if (wave6_vpu_wait_interrupt(inst, VPU_ENC_TIMEOUT) < 0) {
			dev_err(inst->dev->dev, "failed to call wave6_vpu_wait_interrupt()\n");
			goto exit;
		}
		pr_info("start encode:set dynamic change param successfully\n");
		wave6_vpu_enc_complete_seq_update(inst, &initial_info);
	}

	memset(&pic_param, 0, sizeof(struct enc_param));
	memset(&frame_buf, 0, sizeof(struct frame_buffer));

	if (inst->src_fmt.pixelformat == V4L2_PIX_FMT_YUV420 ||
	    inst->src_fmt.pixelformat == V4L2_PIX_FMT_YUV420M)
		chroma_size = ((stride / 2) * (inst->src_fmt.height / 2));
	else if (inst->src_fmt.pixelformat == V4L2_PIX_FMT_YUV422P ||
		 inst->src_fmt.pixelformat == V4L2_PIX_FMT_YUV422M)
		chroma_size = ((stride) * (inst->src_fmt.height / 2));
	else
		chroma_size = 0;

	src_buf = wave6_get_valid_src_buf(inst);
	dst_buf = wave6_get_valid_dst_buf(inst);

	if (!dst_buf) {
		dev_err(inst->dev->dev, "no valid dst buf\n");
		goto exit;
	}

	dst_vbuf = wave6_to_vpu_buf(dst_buf);
	pic_param.pic_stream_buffer_addr = wave6_get_dma_addr(dst_buf, 0);
	pic_param.pic_stream_buffer_size = vb2_plane_size(&dst_buf->vb2_buf, 0);
	if (!src_buf) {
		dev_dbg(inst->dev->dev, "no valid src buf\n");
		if (inst->state == VPU_INST_STATE_STOP)
			pic_param.src_end_flag = 1;
		else
			goto exit;
	} else {
		src_vbuf = wave6_to_vpu_buf(src_buf);
		if (inst->src_fmt.num_planes == 1) {
			frame_buf.buf_y = wave6_get_dma_addr(src_buf, 0);
			frame_buf.buf_cb = frame_buf.buf_y + luma_size;
			frame_buf.buf_cr = frame_buf.buf_cb + chroma_size;
		} else if (inst->src_fmt.num_planes == 2) {
			frame_buf.buf_y = wave6_get_dma_addr(src_buf, 0);
			frame_buf.buf_cb = wave6_get_dma_addr(src_buf, 1);
			frame_buf.buf_cr = frame_buf.buf_cb + chroma_size;
		} else if (inst->src_fmt.num_planes == 3) {
			frame_buf.buf_y = wave6_get_dma_addr(src_buf, 0);
			frame_buf.buf_cb = wave6_get_dma_addr(src_buf, 1);
			frame_buf.buf_cr = wave6_get_dma_addr(src_buf, 2);
		}
		for (int i = 0; i < inst->src_fmt.num_planes; i++) {
			dma_addr_t daddr = vb2_dma_contig_plane_dma_addr(&src_buf->vb2_buf, i);

			if (swiotlb_find_pool(inst->dev->dev, daddr)) {
				dma_sync_single_for_device(inst->dev->dev, daddr,
							   inst->src_fmt.plane_fmt[i].sizeimage,
							   DMA_BIDIRECTIONAL);
			}
		}
		wave6_update_frame_buf_addr(inst, &frame_buf);
		frame_buf.stride = stride;
		pic_param.src_idx = src_buf->vb2_buf.index;
		if (src_vbuf->force_key_frame || inst->error_recovery) {
			pic_param.force_pic_type_enable = true;
			pic_param.force_pic_type = ENC_FORCE_PIC_TYPE_IDR;
			inst->error_recovery = false;
		}
		src_vbuf->ts_start = ktime_get_raw();
	}

	pic_param.source_frame = &frame_buf;
	pic_param.code_option.implicit_header_encode = 1;
	wave6_set_csc(inst, &pic_param);

	if (src_vbuf)
		src_vbuf->consumed = true;
	if (dst_vbuf) {
		dst_vbuf->consumed = true;
		dst_vbuf->used = true;
	}
	ret = wave6_vpu_enc_start_one_frame(inst, &pic_param, &fail_res);
	if (ret) {
		dev_err(inst->dev->dev, "[%d] %s: fail %d\n", inst->id, __func__, ret);
		wave6_vpu_set_instance_state(inst, VPU_INST_STATE_STOP);

		dst_buf = v4l2_m2m_dst_buf_remove(inst->v4l2_fh.m2m_ctx);
		if (dst_buf) {
			dst_buf->sequence = inst->sequence;
			v4l2_m2m_buf_done(dst_buf, VB2_BUF_STATE_ERROR);
		}

		src_buf = v4l2_m2m_src_buf_remove(inst->v4l2_fh.m2m_ctx);
		if (src_buf) {
			v4l2_m2m_buf_done(src_buf, VB2_BUF_STATE_ERROR);
			inst->sequence++;
			inst->processed_buf_num++;
			inst->error_buf_num++;
		}
	} else {
		dev_dbg(inst->dev->dev, "%s: success\n", __func__);
	}

exit:
	return ret;
}

static void wave6_handle_encoded_frame(struct vpu_instance *inst,
				       struct enc_output_info *info)
{
	struct vb2_v4l2_buffer *src_buf;
	struct vb2_v4l2_buffer *dst_buf;
	struct vpu_buffer *vpu_buf;
	struct vpu_buffer *dst_vpu_buf;
	enum vb2_buffer_state state;

	state = info->encoding_success ? VB2_BUF_STATE_DONE : VB2_BUF_STATE_ERROR;

	src_buf = v4l2_m2m_src_buf_remove_by_idx(inst->v4l2_fh.m2m_ctx,
						 info->enc_src_idx);
	if (!src_buf) {
		dev_err(inst->dev->dev, "[%d] encoder can't find src buffer\n", inst->id);
		return;
	}

	vpu_buf = wave6_to_vpu_buf(src_buf);
	if (!vpu_buf || !vpu_buf->consumed) {
		dev_err(inst->dev->dev, "[%d] src buffer is not consumed\n", inst->id);
		return;
	}

	dst_buf = wave6_get_dst_buf_by_addr(inst, info->bitstream_buffer);
	if (!dst_buf) {
		dev_err(inst->dev->dev, "[%d] encoder can't find dst buffer\n", inst->id);
		return;
	}

	dst_vpu_buf = wave6_to_vpu_buf(dst_buf);
	dst_vpu_buf->ts_input = vpu_buf->ts_input;
	dst_vpu_buf->ts_start = vpu_buf->ts_start;
	dst_vpu_buf->ts_finish = ktime_get_raw();
	dst_vpu_buf->hw_time = wave6_cycle_to_ns(inst->dev, info->cycle.frame_cycle);
	dst_vpu_buf->ts_output = ktime_get_raw();
	wave6_vpu_handle_performance(inst, dst_vpu_buf);

	dst_vpu_buf->average_qp = info->avg_ctu_qp;

	v4l2_m2m_buf_copy_metadata(src_buf, dst_buf, true);
	v4l2_m2m_buf_done(src_buf, state);

	vb2_set_plane_payload(&dst_buf->vb2_buf, 0, info->bitstream_size);
	dst_buf->sequence = inst->sequence++;
	dst_buf->field = V4L2_FIELD_NONE;
	if (info->pic_type == PIC_TYPE_I)
		dst_buf->flags |= V4L2_BUF_FLAG_KEYFRAME;
	else if (info->pic_type == PIC_TYPE_P)
		dst_buf->flags |= V4L2_BUF_FLAG_PFRAME;
	else if (info->pic_type == PIC_TYPE_B)
		dst_buf->flags |= V4L2_BUF_FLAG_BFRAME;

	v4l2_m2m_dst_buf_remove_by_buf(inst->v4l2_fh.m2m_ctx, dst_buf);
	if (state == VB2_BUF_STATE_ERROR) {
		dprintk(inst->dev->dev, "[%d] error frame %d\n", inst->id, inst->sequence);
		inst->error_recovery = true;
		inst->error_buf_num++;
	}
	if (swiotlb_find_pool(inst->dev->dev, info->bitstream_buffer))
		dma_sync_single_for_cpu(inst->dev->dev, info->bitstream_buffer,
					info->bitstream_size, DMA_BIDIRECTIONAL);
	v4l2_m2m_buf_done(dst_buf, state);
	inst->processed_buf_num++;

	inst->total_frames++;
	inst->total_frame_cycle += info->cycle.frame_cycle;
}

static void wave6_handle_last_frame(struct vpu_instance *inst,
				    dma_addr_t addr)
{
	struct vb2_v4l2_buffer *dst_buf;

	dst_buf = wave6_get_dst_buf_by_addr(inst, addr);
	if (!dst_buf)
		return;

	vb2_set_plane_payload(&dst_buf->vb2_buf, 0, 0);
	dst_buf->field = V4L2_FIELD_NONE;
	dst_buf->flags |= V4L2_BUF_FLAG_LAST;
	v4l2_m2m_dst_buf_remove_by_buf(inst->v4l2_fh.m2m_ctx, dst_buf);
	v4l2_m2m_buf_done(dst_buf, VB2_BUF_STATE_DONE);

	wave6_vpu_set_instance_state(inst, VPU_INST_STATE_PIC_RUN);

	dprintk(inst->dev->dev, "[%d] eos\n", inst->id);
	inst->eos = true;

	v4l2_m2m_set_src_buffered(inst->v4l2_fh.m2m_ctx, false);

	if (inst->total_frames && inst->total_frame_cycle) {
		dprintk(inst->dev->dev, "total frames %llu,avg cycle %llu,fps %llu\n",
			inst->total_frames,
			(inst->total_frame_cycle / inst->total_frames),
			(666000000 * inst->total_frames / inst->total_frame_cycle));
	} else {
		dprintk(inst->dev->dev, "no frame encode done!\n");
	}
}

static void wave6_vpu_enc_finish_encode(struct vpu_instance *inst, int irq_status)
{
	int ret;
	struct enc_output_info info = { 0 };

	if (irq_status & BIT(INT_WAVE6_BSBUF_FULL)) {
		dev_err(inst->dev->dev, "Unexpected irq 0x%x\n", irq_status);

		vb2_queue_error(v4l2_m2m_get_src_vq(inst->v4l2_fh.m2m_ctx));
		vb2_queue_error(v4l2_m2m_get_dst_vq(inst->v4l2_fh.m2m_ctx));

		wave6_vpu_set_instance_state(inst, VPU_INST_STATE_STOP);
		inst->eos = true;

		goto finish_encode;
	}

	ret = wave6_vpu_enc_get_output_info(inst, &info);
	if (ret) {
		dev_err(inst->dev->dev, "vpu_enc_get_output_info fail %d  reason: %d | info : %d\n",
			ret, info.error_reason, info.warn_info);
		goto finish_encode;
	}

	if (info.enc_src_idx >= 0 && info.recon_frame_index >= 0)
		wave6_handle_encoded_frame(inst, &info);
	else if (info.recon_frame_index == RECON_IDX_FLAG_ENC_END)
		wave6_handle_last_frame(inst, info.bitstream_buffer);

finish_encode:
	wave6_vpu_finish_job(inst);
}

static int wave6_vpu_enc_querycap(struct file *file, void *fh, struct v4l2_capability *cap)
{
	strscpy(cap->driver, VPU_ENC_DRV_NAME, sizeof(cap->driver));
	strscpy(cap->card, VPU_ENC_DRV_NAME, sizeof(cap->card));
	strscpy(cap->bus_info, "platform:" VPU_ENC_DRV_NAME, sizeof(cap->bus_info));

	return 0;
}

static int wave6_vpu_enc_enum_framesizes(struct file *f, void *fh, struct v4l2_frmsizeenum *fsize)
{
	const struct vpu_format *vpu_fmt;

	if (fsize->index)
		return -EINVAL;

	vpu_fmt = wave6_find_vpu_fmt(fsize->pixel_format, VPU_FMT_TYPE_CODEC);
	if (!vpu_fmt) {
		vpu_fmt = wave6_find_vpu_fmt(fsize->pixel_format, VPU_FMT_TYPE_RAW);
		if (!vpu_fmt)
			return -EINVAL;
	}

	fsize->type = V4L2_FRMSIZE_TYPE_STEPWISE;
	fsize->stepwise.min_width = vpu_fmt->min_width;
	fsize->stepwise.max_width = vpu_fmt->max_width;
	fsize->stepwise.step_width = W6_ENC_PIC_SIZE_STEP;
	fsize->stepwise.min_height = vpu_fmt->min_height;
	fsize->stepwise.max_height = vpu_fmt->max_height;
	fsize->stepwise.step_height = W6_ENC_PIC_SIZE_STEP;

	return 0;
}

static int wave6_vpu_enc_enum_fmt_cap(struct file *file, void *fh, struct v4l2_fmtdesc *f)
{
	struct vpu_instance *inst = wave6_to_vpu_inst(fh);
	const struct vpu_format *vpu_fmt;

	dev_dbg(inst->dev->dev, "index : %d\n", f->index);

	vpu_fmt = wave6_find_vpu_fmt_by_idx(f->index, VPU_FMT_TYPE_CODEC);
	if (!vpu_fmt)
		return -EINVAL;

	f->pixelformat = vpu_fmt->v4l2_pix_fmt;
	f->flags = 0;

	return 0;
}

static int wave6_vpu_enc_try_fmt_cap(struct file *file, void *fh, struct v4l2_format *f)
{
	struct vpu_instance *inst = wave6_to_vpu_inst(fh);
	struct v4l2_pix_format_mplane *pix_mp = &f->fmt.pix_mp;
	const struct vpu_format *vpu_fmt;
	int width, height;

	dev_dbg(inst->dev->dev, "%s: 4cc %d w %d h %d plane %d colorspace %d\n",
		__func__, pix_mp->pixelformat, pix_mp->width, pix_mp->height,
		pix_mp->num_planes, pix_mp->colorspace);

	if (!V4L2_TYPE_IS_CAPTURE(f->type))
		return -EINVAL;

	vpu_fmt = wave6_find_vpu_fmt(pix_mp->pixelformat, VPU_FMT_TYPE_CODEC);
	if (!vpu_fmt) {
		width = inst->dst_fmt.width;
		height = inst->dst_fmt.height;
		pix_mp->pixelformat = inst->dst_fmt.pixelformat;
		pix_mp->num_planes = inst->dst_fmt.num_planes;
	} else {
		width = pix_mp->width;
		height = pix_mp->height;
		pix_mp->pixelformat = vpu_fmt->v4l2_pix_fmt;
		pix_mp->num_planes = vpu_fmt->num_planes;
	}

	wave6_update_pix_fmt(pix_mp, width, height);
	pix_mp->colorspace = inst->colorspace;
	pix_mp->ycbcr_enc = inst->ycbcr_enc;
	pix_mp->quantization = inst->quantization;
	pix_mp->xfer_func = inst->xfer_func;

	return 0;
}

static int wave6_vpu_enc_s_fmt_cap(struct file *file, void *fh, struct v4l2_format *f)
{
	struct vpu_instance *inst = wave6_to_vpu_inst(fh);
	struct v4l2_pix_format_mplane *pix_mp = &f->fmt.pix_mp;
	int i, ret;

	dev_dbg(inst->dev->dev, "%s: 4cc %d w %d h %d plane %d colorspace %d\n",
		__func__, pix_mp->pixelformat, pix_mp->width, pix_mp->height,
		pix_mp->num_planes, pix_mp->colorspace);

	ret = wave6_vpu_enc_try_fmt_cap(file, fh, f);
	if (ret)
		return ret;

	inst->dst_fmt.width = pix_mp->width;
	inst->dst_fmt.height = pix_mp->height;
	inst->dst_fmt.pixelformat = pix_mp->pixelformat;
	inst->dst_fmt.field = pix_mp->field;
	inst->dst_fmt.flags = pix_mp->flags;
	inst->dst_fmt.num_planes = pix_mp->num_planes;
	for (i = 0; i < inst->dst_fmt.num_planes; i++) {
		inst->dst_fmt.plane_fmt[i].bytesperline = pix_mp->plane_fmt[i].bytesperline;
		inst->dst_fmt.plane_fmt[i].sizeimage = pix_mp->plane_fmt[i].sizeimage;
	}

	return 0;
}

static int wave6_vpu_enc_g_fmt_cap(struct file *file, void *fh, struct v4l2_format *f)
{
	struct vpu_instance *inst = wave6_to_vpu_inst(fh);
	struct v4l2_pix_format_mplane *pix_mp = &f->fmt.pix_mp;
	int i;

	pix_mp->width = inst->dst_fmt.width;
	pix_mp->height = inst->dst_fmt.height;
	pix_mp->pixelformat = inst->dst_fmt.pixelformat;
	pix_mp->field = inst->dst_fmt.field;
	pix_mp->flags = inst->dst_fmt.flags;
	pix_mp->num_planes = inst->dst_fmt.num_planes;
	for (i = 0; i < pix_mp->num_planes; i++) {
		pix_mp->plane_fmt[i].bytesperline = inst->dst_fmt.plane_fmt[i].bytesperline;
		pix_mp->plane_fmt[i].sizeimage = inst->dst_fmt.plane_fmt[i].sizeimage;
	}

	pix_mp->colorspace = inst->colorspace;
	pix_mp->ycbcr_enc = inst->ycbcr_enc;
	pix_mp->quantization = inst->quantization;
	pix_mp->xfer_func = inst->xfer_func;

	return 0;
}

static int wave6_vpu_enc_enum_fmt_out(struct file *file, void *fh, struct v4l2_fmtdesc *f)
{
	struct vpu_instance *inst = wave6_to_vpu_inst(fh);
	const struct vpu_format *vpu_fmt;

	dev_dbg(inst->dev->dev, "%s: index %d\n", __func__, f->index);

	vpu_fmt = wave6_find_vpu_fmt_by_idx(f->index, VPU_FMT_TYPE_RAW);
	if (!vpu_fmt)
		return -EINVAL;

	f->pixelformat = vpu_fmt->v4l2_pix_fmt;
	f->flags = 0;

	return 0;
}

static int wave6_vpu_enc_try_fmt_out(struct file *file, void *fh, struct v4l2_format *f)
{
	struct vpu_instance *inst = wave6_to_vpu_inst(fh);
	struct v4l2_pix_format_mplane *pix_mp = &f->fmt.pix_mp;
	const struct vpu_format *vpu_fmt;
	int width, height;

	dev_dbg(inst->dev->dev, "%s: 4cc %d w %d h %d plane %d colorspace %d\n",
		__func__, pix_mp->pixelformat, pix_mp->width, pix_mp->height,
		pix_mp->num_planes, pix_mp->colorspace);

	if (!V4L2_TYPE_IS_OUTPUT(f->type))
		return -EINVAL;

	vpu_fmt = wave6_find_vpu_fmt(pix_mp->pixelformat, VPU_FMT_TYPE_RAW);
	if (!vpu_fmt) {
		width = inst->src_fmt.width;
		height = inst->src_fmt.height;
		pix_mp->pixelformat = inst->src_fmt.pixelformat;
		pix_mp->num_planes = inst->src_fmt.num_planes;
	} else {
		width = clamp(pix_mp->width,
			      vpu_fmt->min_width, vpu_fmt->max_width);
		height = clamp(pix_mp->height,
			       vpu_fmt->min_height, vpu_fmt->max_height);

		pix_mp->pixelformat = vpu_fmt->v4l2_pix_fmt;
		pix_mp->num_planes = vpu_fmt->num_planes;
	}

	wave6_update_pix_fmt(pix_mp, width, height);

	if (pix_mp->ycbcr_enc == V4L2_YCBCR_ENC_BT2020_CONST_LUM)
		pix_mp->ycbcr_enc = V4L2_YCBCR_ENC_BT2020;
	if (pix_mp->ycbcr_enc == V4L2_YCBCR_ENC_XV601 ||
	    pix_mp->ycbcr_enc == V4L2_YCBCR_ENC_XV709) {
		if (pix_mp->quantization == V4L2_QUANTIZATION_FULL_RANGE)
			pix_mp->quantization = V4L2_QUANTIZATION_LIM_RANGE;
	}

	return 0;
}

static int wave6_vpu_enc_s_fmt_out(struct file *file, void *fh, struct v4l2_format *f)
{
	struct vpu_instance *inst = wave6_to_vpu_inst(fh);
	struct v4l2_pix_format_mplane *pix_mp = &f->fmt.pix_mp;
	int i, ret;

	dev_dbg(inst->dev->dev, "%s: 4cc %d w %d h %d plane %d colorspace %d\n",
		__func__, pix_mp->pixelformat, pix_mp->width, pix_mp->height,
		pix_mp->num_planes, pix_mp->colorspace);

	ret = wave6_vpu_enc_try_fmt_out(file, fh, f);
	if (ret)
		return ret;

	inst->src_fmt.width = pix_mp->width;
	inst->src_fmt.height = pix_mp->height;
	inst->src_fmt.pixelformat = pix_mp->pixelformat;
	inst->src_fmt.field = pix_mp->field;
	inst->src_fmt.flags = pix_mp->flags;
	inst->src_fmt.num_planes = pix_mp->num_planes;
	for (i = 0; i < inst->src_fmt.num_planes; i++) {
		inst->src_fmt.plane_fmt[i].bytesperline = pix_mp->plane_fmt[i].bytesperline;
		inst->src_fmt.plane_fmt[i].sizeimage = pix_mp->plane_fmt[i].sizeimage;
	}

	if (inst->src_fmt.pixelformat == V4L2_PIX_FMT_NV12 ||
	    inst->src_fmt.pixelformat == V4L2_PIX_FMT_NV16 ||
	    inst->src_fmt.pixelformat == V4L2_PIX_FMT_NV24 ||
	    inst->src_fmt.pixelformat == V4L2_PIX_FMT_NV12M ||
	    inst->src_fmt.pixelformat == V4L2_PIX_FMT_NV16M ||
	    inst->src_fmt.pixelformat == V4L2_PIX_FMT_RGB24 ||
	    inst->src_fmt.pixelformat == V4L2_PIX_FMT_YUV24 ||
	    inst->src_fmt.pixelformat == V4L2_PIX_FMT_P010 ||
	    inst->src_fmt.pixelformat == V4L2_PIX_FMT_ARGB32 ||
	    inst->src_fmt.pixelformat == V4L2_PIX_FMT_XRGB32 ||
	    inst->src_fmt.pixelformat == V4L2_PIX_FMT_RGBA32 ||
	    inst->src_fmt.pixelformat == V4L2_PIX_FMT_RGBX32 ||
	    inst->src_fmt.pixelformat == V4L2_PIX_FMT_ARGB2101010) {
		inst->cbcr_interleave = true;
		inst->nv21 = false;
	} else if (inst->src_fmt.pixelformat == V4L2_PIX_FMT_NV21 ||
		   inst->src_fmt.pixelformat == V4L2_PIX_FMT_NV61 ||
		   inst->src_fmt.pixelformat == V4L2_PIX_FMT_NV42 ||
		   inst->src_fmt.pixelformat == V4L2_PIX_FMT_NV21M ||
		   inst->src_fmt.pixelformat == V4L2_PIX_FMT_NV61M) {
		inst->cbcr_interleave = true;
		inst->nv21 = true;
	} else {
		inst->cbcr_interleave = false;
		inst->nv21 = false;
	}

	inst->colorspace = pix_mp->colorspace;
	inst->ycbcr_enc = pix_mp->ycbcr_enc;
	inst->quantization = pix_mp->quantization;
	inst->xfer_func = pix_mp->xfer_func;

	wave6_update_pix_fmt(&inst->dst_fmt, pix_mp->width, pix_mp->height);
	inst->crop.left = 0;
	inst->crop.top = 0;
	inst->crop.width = inst->dst_fmt.width;
	inst->crop.height = inst->dst_fmt.height;

	return 0;
}

static int wave6_vpu_enc_g_fmt_out(struct file *file, void *fh, struct v4l2_format *f)
{
	struct vpu_instance *inst = wave6_to_vpu_inst(fh);
	struct v4l2_pix_format_mplane *pix_mp = &f->fmt.pix_mp;
	int i;

	dev_dbg(inst->dev->dev, "\n");

	pix_mp->width = inst->src_fmt.width;
	pix_mp->height = inst->src_fmt.height;
	pix_mp->pixelformat = inst->src_fmt.pixelformat;
	pix_mp->field = inst->src_fmt.field;
	pix_mp->flags = inst->src_fmt.flags;
	pix_mp->num_planes = inst->src_fmt.num_planes;
	for (i = 0; i < pix_mp->num_planes; i++) {
		pix_mp->plane_fmt[i].bytesperline = inst->src_fmt.plane_fmt[i].bytesperline;
		pix_mp->plane_fmt[i].sizeimage = inst->src_fmt.plane_fmt[i].sizeimage;
	}

	pix_mp->colorspace = inst->colorspace;
	pix_mp->ycbcr_enc = inst->ycbcr_enc;
	pix_mp->quantization = inst->quantization;
	pix_mp->xfer_func = inst->xfer_func;

	return 0;
}

static int wave6_vpu_enc_g_selection(struct file *file, void *fh, struct v4l2_selection *s)
{
	struct vpu_instance *inst = wave6_to_vpu_inst(fh);

	dev_dbg(inst->dev->dev, "%s: type %d target %d\n",
		__func__, s->type, s->target);

	if (!V4L2_TYPE_IS_OUTPUT(s->type))
		return -EINVAL;

	switch (s->target) {
	case V4L2_SEL_TGT_CROP_DEFAULT:
	case V4L2_SEL_TGT_CROP_BOUNDS:
		s->r.left = 0;
		s->r.top = 0;
		s->r.width = inst->src_fmt.width;
		s->r.height = inst->src_fmt.height;
		break;
	case V4L2_SEL_TGT_CROP:
		s->r = inst->crop;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int wave6_vpu_enc_s_selection(struct file *file, void *fh, struct v4l2_selection *s)
{
	struct vpu_instance *inst = wave6_to_vpu_inst(fh);
	u32 max_crop_w, max_crop_h;

	if (!V4L2_TYPE_IS_OUTPUT(s->type))
		return -EINVAL;

	if (s->target != V4L2_SEL_TGT_CROP)
		return -EINVAL;

	if (!(s->flags & (V4L2_SEL_FLAG_GE | V4L2_SEL_FLAG_LE)))
		s->flags |= V4L2_SEL_FLAG_LE;

	if (s->flags & V4L2_SEL_FLAG_GE) {
		s->r.left = round_up(s->r.left, W6_ENC_CROP_X_POS_STEP);
		s->r.top = round_up(s->r.top, W6_ENC_CROP_Y_POS_STEP);
		s->r.width = round_up(s->r.width, W6_ENC_PIC_SIZE_STEP);
		s->r.height = round_up(s->r.height, W6_ENC_PIC_SIZE_STEP);
	}
	if (s->flags & V4L2_SEL_FLAG_LE) {
		s->r.left = round_down(s->r.left, W6_ENC_CROP_X_POS_STEP);
		s->r.top = round_down(s->r.top, W6_ENC_CROP_Y_POS_STEP);
		s->r.width = round_down(s->r.width, W6_ENC_PIC_SIZE_STEP);
		s->r.height = round_down(s->r.height, W6_ENC_PIC_SIZE_STEP);
	}

	max_crop_w = inst->src_fmt.width - s->r.left;
	max_crop_h = inst->src_fmt.height - s->r.top;

	if (s->r.width == 0 || s->r.height == 0)
		return 0;
	if (max_crop_w < W6_MIN_ENC_PIC_WIDTH)
		return 0;
	if (max_crop_h < W6_MIN_ENC_PIC_HEIGHT)
		return 0;

	s->r.width = clamp(s->r.width, W6_MIN_ENC_PIC_WIDTH, max_crop_w);
	s->r.height = clamp(s->r.height, W6_MIN_ENC_PIC_HEIGHT, max_crop_h);

	inst->crop = s->r;
	wave6_update_pix_fmt(&inst->dst_fmt, s->r.width, s->r.height);

	dev_dbg(inst->dev->dev, "V4L2_SEL_TGT_CROP %dx%dx%dx%d\n",
		s->r.left, s->r.top, s->r.width, s->r.height);

	return 0;
}

static int wave6_vpu_enc_encoder_cmd(struct file *file, void *fh, struct v4l2_encoder_cmd *ec)
{
	struct vpu_instance *inst = wave6_to_vpu_inst(fh);
	int ret;

	dev_dbg(inst->dev->dev, "%s: cmd %d\n", __func__, ec->cmd);

	ret = v4l2_m2m_ioctl_try_encoder_cmd(file, fh, ec);
	if (ret)
		return ret;

	if (!wave6_vpu_both_queues_are_streaming(inst))
		return 0;

	switch (ec->cmd) {
	case V4L2_ENC_CMD_STOP:
		wave6_vpu_set_instance_state(inst, VPU_INST_STATE_STOP);
		v4l2_m2m_set_src_buffered(inst->v4l2_fh.m2m_ctx, true);
		v4l2_m2m_try_schedule(inst->v4l2_fh.m2m_ctx);
		break;
	case V4L2_ENC_CMD_START:
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int wave6_vpu_enc_g_parm(struct file *file, void *fh, struct v4l2_streamparm *a)
{
	struct vpu_instance *inst = wave6_to_vpu_inst(fh);

	dev_dbg(inst->dev->dev, "%s: type %d\n", __func__, a->type);

	if (!V4L2_TYPE_IS_OUTPUT(a->type))
		return -EINVAL;

	a->parm.output.capability = V4L2_CAP_TIMEPERFRAME;
	a->parm.output.timeperframe.numerator = 1;
	a->parm.output.timeperframe.denominator = inst->frame_rate;

	dev_dbg(inst->dev->dev, "%s: numerator : %d | denominator : %d\n",
		__func__,
		a->parm.output.timeperframe.numerator,
		a->parm.output.timeperframe.denominator);

	return 0;
}

static int wave6_vpu_enc_s_parm(struct file *file, void *fh, struct v4l2_streamparm *a)
{
	struct vpu_instance *inst = wave6_to_vpu_inst(fh);

	dev_dbg(inst->dev->dev, "%s: type %d\n", __func__, a->type);

	if (!V4L2_TYPE_IS_OUTPUT(a->type))
		return -EINVAL;

	a->parm.output.capability = V4L2_CAP_TIMEPERFRAME;
	if (a->parm.output.timeperframe.denominator && a->parm.output.timeperframe.numerator) {
		inst->frame_rate = a->parm.output.timeperframe.denominator /
				   a->parm.output.timeperframe.numerator;
	} else {
		a->parm.output.timeperframe.numerator = 1;
		a->parm.output.timeperframe.denominator = inst->frame_rate;
	}

	dev_dbg(inst->dev->dev, "%s: numerator : %d | denominator : %d\n",
		__func__,
		a->parm.output.timeperframe.numerator,
		a->parm.output.timeperframe.denominator);

	return 0;
}

static const struct v4l2_ioctl_ops wave6_vpu_enc_ioctl_ops = {
	.vidioc_querycap = wave6_vpu_enc_querycap,
	.vidioc_enum_framesizes = wave6_vpu_enc_enum_framesizes,

	.vidioc_enum_fmt_vid_cap = wave6_vpu_enc_enum_fmt_cap,
	.vidioc_s_fmt_vid_cap_mplane = wave6_vpu_enc_s_fmt_cap,
	.vidioc_g_fmt_vid_cap_mplane = wave6_vpu_enc_g_fmt_cap,
	.vidioc_try_fmt_vid_cap_mplane = wave6_vpu_enc_try_fmt_cap,

	.vidioc_enum_fmt_vid_out = wave6_vpu_enc_enum_fmt_out,
	.vidioc_s_fmt_vid_out_mplane = wave6_vpu_enc_s_fmt_out,
	.vidioc_g_fmt_vid_out_mplane = wave6_vpu_enc_g_fmt_out,
	.vidioc_try_fmt_vid_out_mplane = wave6_vpu_enc_try_fmt_out,

	.vidioc_g_selection = wave6_vpu_enc_g_selection,
	.vidioc_s_selection = wave6_vpu_enc_s_selection,

	.vidioc_g_parm = wave6_vpu_enc_g_parm,
	.vidioc_s_parm = wave6_vpu_enc_s_parm,

	.vidioc_reqbufs = v4l2_m2m_ioctl_reqbufs,
	.vidioc_querybuf = v4l2_m2m_ioctl_querybuf,
	.vidioc_create_bufs = v4l2_m2m_ioctl_create_bufs,
	.vidioc_prepare_buf = v4l2_m2m_ioctl_prepare_buf,
	.vidioc_qbuf = v4l2_m2m_ioctl_qbuf,
	.vidioc_expbuf = v4l2_m2m_ioctl_expbuf,
	.vidioc_dqbuf = v4l2_m2m_ioctl_dqbuf,
	.vidioc_streamon = v4l2_m2m_ioctl_streamon,
	.vidioc_streamoff = v4l2_m2m_ioctl_streamoff,

	.vidioc_try_encoder_cmd = v4l2_m2m_ioctl_try_encoder_cmd,
	.vidioc_encoder_cmd = wave6_vpu_enc_encoder_cmd,

	.vidioc_subscribe_event = wave6_vpu_subscribe_event,
	.vidioc_unsubscribe_event = v4l2_event_unsubscribe,
};

static int wave6_vpu_enc_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct vpu_instance *inst = wave6_ctrl_to_vpu_inst(ctrl);
	struct enc_controls *p = &inst->enc_ctrls;

	dev_dbg(inst->dev->dev, "%s: name %s value %d\n",
		__func__, ctrl->name, ctrl->val);

	switch (ctrl->id) {
	case V4L2_CID_HFLIP:
		p->mirror_direction |= (ctrl->val << 1);
		break;
	case V4L2_CID_VFLIP:
		p->mirror_direction |= ctrl->val;
		break;
	case V4L2_CID_ROTATE:
		p->rot_angle = ctrl->val;
		break;
	case V4L2_CID_MIN_BUFFERS_FOR_OUTPUT:
		break;
	case V4L2_CID_MPEG_VIDEO_GOP_SIZE:
		p->gop_size = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MODE:
		p->slice_mode = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_MB:
		p->slice_max_mb = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_BITRATE_MODE:
		p->bitrate_mode = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_BITRATE:
		p->bitrate = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_FRAME_RC_ENABLE:
		p->frame_rc_enable = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_MB_RC_ENABLE:
		p->mb_rc_enable = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_FORCE_KEY_FRAME:
		p->force_key_frame = true;
		break;
	case V4L2_CID_MPEG_VIDEO_PREPEND_SPSPPS_TO_IDR:
		p->prepend_spspps_to_idr = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_INTRA_REFRESH_PERIOD_TYPE:
		break;
	case V4L2_CID_MPEG_VIDEO_INTRA_REFRESH_PERIOD:
		p->intra_refresh_period = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_PROFILE:
		p->hevc.profile = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_LEVEL:
		p->hevc.level = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_MIN_QP:
		p->hevc.min_qp = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_MAX_QP:
		p->hevc.max_qp = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_I_FRAME_QP:
		p->hevc.i_frame_qp = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_LOOP_FILTER_MODE:
		p->hevc.loop_filter_mode = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_LF_BETA_OFFSET_DIV2:
		p->hevc.lf_beta_offset_div2 = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_LF_TC_OFFSET_DIV2:
		p->hevc.lf_tc_offset_div2 = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_REFRESH_TYPE:
		p->hevc.refresh_type = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_REFRESH_PERIOD:
		p->hevc.refresh_period = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_CONST_INTRA_PRED:
		p->hevc.const_intra_pred = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_STRONG_SMOOTHING:
		p->hevc.strong_smoothing = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_TMV_PREDICTION:
		p->hevc.tmv_prediction = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_PROFILE:
		p->h264.profile = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_LEVEL:
		p->h264.level = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_MIN_QP:
		p->h264.min_qp = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_MAX_QP:
		p->h264.max_qp = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_I_FRAME_QP:
		p->h264.i_frame_qp = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_MODE:
		p->h264.loop_filter_mode = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_BETA:
		p->h264.loop_filter_beta = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_ALPHA:
		p->h264.loop_filter_alpha = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_8X8_TRANSFORM:
		p->h264._8x8_transform = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_CONSTRAINED_INTRA_PREDICTION:
		p->h264.constrained_intra_prediction = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_CHROMA_QP_INDEX_OFFSET:
		p->h264.chroma_qp_index_offset = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_ENTROPY_MODE:
		p->h264.entropy_mode = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_I_PERIOD:
		p->h264.i_period = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_VUI_SAR_ENABLE:
		p->h264.vui_sar_enable = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_VUI_SAR_IDC:
		p->h264.vui_sar_idc = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_VUI_EXT_SAR_WIDTH:
		p->h264.vui_ext_sar_width = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_VUI_EXT_SAR_HEIGHT:
		p->h264.vui_ext_sar_height = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_CPB_SIZE:
		p->h264.cpb_size = ctrl->val;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct v4l2_ctrl_ops wave6_vpu_enc_ctrl_ops = {
	.s_ctrl = wave6_vpu_enc_s_ctrl,
};

static u32 to_video_full_range_flag(enum v4l2_quantization quantization)
{
	switch (quantization) {
	case V4L2_QUANTIZATION_FULL_RANGE:
		return 1;
	case V4L2_QUANTIZATION_LIM_RANGE:
	default:
		return 0;
	}
}

static u32 to_colour_primaries(enum v4l2_colorspace colorspace)
{
	switch (colorspace) {
	case V4L2_COLORSPACE_SMPTE170M:
		return 6;
	case V4L2_COLORSPACE_REC709:
	case V4L2_COLORSPACE_SRGB:
	case V4L2_COLORSPACE_JPEG:
		return 1;
	case V4L2_COLORSPACE_BT2020:
		return 9;
	case V4L2_COLORSPACE_DCI_P3:
		return 11;
	case V4L2_COLORSPACE_SMPTE240M:
		return 7;
	case V4L2_COLORSPACE_470_SYSTEM_M:
		return 4;
	case V4L2_COLORSPACE_470_SYSTEM_BG:
		return 5;
	case V4L2_COLORSPACE_RAW:
	default:
		return 2;
	}
}

static u32 to_transfer_characteristics(enum v4l2_colorspace colorspace,
				       enum v4l2_xfer_func xfer_func)
{
	if (xfer_func == V4L2_XFER_FUNC_DEFAULT)
		xfer_func = V4L2_MAP_XFER_FUNC_DEFAULT(colorspace);

	switch (xfer_func) {
	case V4L2_XFER_FUNC_709:
		if (colorspace == V4L2_COLORSPACE_SMPTE170M)
			return 6;
		else if (colorspace == V4L2_COLORSPACE_BT2020)
			return 14;
		else
			return 1;
	case V4L2_XFER_FUNC_SRGB:
		return 13;
	case V4L2_XFER_FUNC_SMPTE240M:
		return 7;
	case V4L2_XFER_FUNC_NONE:
		return 8;
	case V4L2_XFER_FUNC_SMPTE2084:
		return 16;
	case V4L2_XFER_FUNC_DCI_P3:
	default:
		return 2;
	}
}

static u32 to_matrix_coeffs(enum v4l2_colorspace colorspace,
			    enum v4l2_ycbcr_encoding ycbcr_enc)
{
	if (ycbcr_enc == V4L2_YCBCR_ENC_DEFAULT)
		ycbcr_enc = V4L2_MAP_YCBCR_ENC_DEFAULT(colorspace);

	switch (ycbcr_enc) {
	case V4L2_YCBCR_ENC_601:
	case V4L2_YCBCR_ENC_XV601:
		if (colorspace == V4L2_COLORSPACE_SMPTE170M)
			return 6;
		else
			return 5;
	case V4L2_YCBCR_ENC_709:
	case V4L2_YCBCR_ENC_XV709:
		return 1;
	case V4L2_YCBCR_ENC_BT2020:
		return 9;
	case V4L2_YCBCR_ENC_BT2020_CONST_LUM:
		return 10;
	case V4L2_YCBCR_ENC_SMPTE240M:
		return 7;
	default:
		return 2;
	}
}

static void wave6_set_enc_h264_param(struct enc_wave_param *output,
				     struct h264_enc_controls *ctrls)
{
	switch (ctrls->profile) {
	case V4L2_MPEG_VIDEO_H264_PROFILE_BASELINE:
	case V4L2_MPEG_VIDEO_H264_PROFILE_CONSTRAINED_BASELINE:
		output->profile = H264_PROFILE_BP;
		output->internal_bit_depth = 8;
		break;
	case V4L2_MPEG_VIDEO_H264_PROFILE_MAIN:
		output->profile = H264_PROFILE_MP;
		output->internal_bit_depth = 8;
		break;
	case V4L2_MPEG_VIDEO_H264_PROFILE_EXTENDED:
		output->profile = H264_PROFILE_EXTENDED;
		output->internal_bit_depth = 8;
		break;
	case V4L2_MPEG_VIDEO_H264_PROFILE_HIGH:
		output->profile = H264_PROFILE_HP;
		output->internal_bit_depth = 8;
		break;
	default:
		break;
	}
	switch (ctrls->level) {
	case V4L2_MPEG_VIDEO_H264_LEVEL_1_0:
		output->level = 10;
		break;
	case V4L2_MPEG_VIDEO_H264_LEVEL_1B:
		output->level = 9;
		break;
	case V4L2_MPEG_VIDEO_H264_LEVEL_1_1:
		output->level = 11;
		break;
	case V4L2_MPEG_VIDEO_H264_LEVEL_1_2:
		output->level = 12;
		break;
	case V4L2_MPEG_VIDEO_H264_LEVEL_1_3:
		output->level = 13;
		break;
	case V4L2_MPEG_VIDEO_H264_LEVEL_2_0:
		output->level = 20;
		break;
	case V4L2_MPEG_VIDEO_H264_LEVEL_2_1:
		output->level = 21;
		break;
	case V4L2_MPEG_VIDEO_H264_LEVEL_2_2:
		output->level = 22;
		break;
	case V4L2_MPEG_VIDEO_H264_LEVEL_3_0:
		output->level = 30;
		break;
	case V4L2_MPEG_VIDEO_H264_LEVEL_3_1:
		output->level = 31;
		break;
	case V4L2_MPEG_VIDEO_H264_LEVEL_3_2:
		output->level = 32;
		break;
	case V4L2_MPEG_VIDEO_H264_LEVEL_4_0:
		output->level = 40;
		break;
	case V4L2_MPEG_VIDEO_H264_LEVEL_4_1:
		output->level = 41;
		break;
	case V4L2_MPEG_VIDEO_H264_LEVEL_4_2:
		output->level = 42;
		break;
	case V4L2_MPEG_VIDEO_H264_LEVEL_5_0:
		output->level = 50;
		break;
	case V4L2_MPEG_VIDEO_H264_LEVEL_5_1:
		output->level = 51;
		break;
	case V4L2_MPEG_VIDEO_H264_LEVEL_5_2:
		output->level = 52;
		break;
	default:
		break;
	}
	output->qp = ctrls->i_frame_qp;
	output->min_qp_i = ctrls->min_qp;
	output->max_qp_i = ctrls->max_qp;
	output->min_qp_p = ctrls->min_qp;
	output->max_qp_p = ctrls->max_qp;
	output->min_qp_b = ctrls->min_qp;
	output->max_qp_b = ctrls->max_qp;
	switch (ctrls->loop_filter_mode) {
	case V4L2_MPEG_VIDEO_H264_LOOP_FILTER_MODE_DISABLED:
		output->en_dbk = 0;
		output->en_lf_cross_slice_boundary = 0;
		break;
	case V4L2_MPEG_VIDEO_H264_LOOP_FILTER_MODE_ENABLED:
		output->en_dbk = 1;
		output->en_lf_cross_slice_boundary = 1;
		break;
	case V4L2_MPEG_VIDEO_H264_LOOP_FILTER_MODE_DISABLED_AT_SLICE_BOUNDARY:
		output->en_dbk = 1;
		output->en_lf_cross_slice_boundary = 0;
		break;
	default:
		break;
	}
	output->intra_period = ctrls->i_period;
	output->beta_offset_div2 = ctrls->loop_filter_beta;
	output->tc_offset_div2 = ctrls->loop_filter_alpha;
	if (output->profile >= H264_PROFILE_HP)
		output->en_transform8x8 = ctrls->_8x8_transform;
	output->en_constrained_intra_pred = ctrls->constrained_intra_prediction;
	output->cb_qp_offset = ctrls->chroma_qp_index_offset;
	output->cr_qp_offset = ctrls->chroma_qp_index_offset;
	if (output->profile >= H264_PROFILE_MP)
		output->en_cabac = ctrls->entropy_mode;
}

static void wave6_set_enc_hevc_param(struct enc_wave_param *output,
				     struct hevc_enc_controls *ctrls)
{
	switch (ctrls->profile) {
	case V4L2_MPEG_VIDEO_HEVC_PROFILE_MAIN:
		output->profile = HEVC_PROFILE_MAIN;
		output->internal_bit_depth = 8;
		break;
	default:
		break;
	}
	switch (ctrls->level) {
	case V4L2_MPEG_VIDEO_HEVC_LEVEL_1:
		output->level = 10 * 3;
		break;
	case V4L2_MPEG_VIDEO_HEVC_LEVEL_2:
		output->level = 20 * 3;
		break;
	case V4L2_MPEG_VIDEO_HEVC_LEVEL_2_1:
		output->level = 21 * 3;
		break;
	case V4L2_MPEG_VIDEO_HEVC_LEVEL_3:
		output->level = 30 * 3;
		break;
	case V4L2_MPEG_VIDEO_HEVC_LEVEL_3_1:
		output->level = 31 * 3;
		break;
	case V4L2_MPEG_VIDEO_HEVC_LEVEL_4:
		output->level = 40 * 3;
		break;
	case V4L2_MPEG_VIDEO_HEVC_LEVEL_4_1:
		output->level = 41 * 3;
		break;
	case V4L2_MPEG_VIDEO_HEVC_LEVEL_5:
		output->level = 50 * 3;
		break;
	case V4L2_MPEG_VIDEO_HEVC_LEVEL_5_1:
		output->level = 51 * 3;
		break;
	default:
		break;
	}
	output->qp = ctrls->i_frame_qp;
	output->min_qp_i = ctrls->min_qp;
	output->max_qp_i = ctrls->max_qp;
	output->min_qp_p = ctrls->min_qp;
	output->max_qp_p = ctrls->max_qp;
	output->min_qp_b = ctrls->min_qp;
	output->max_qp_b = ctrls->max_qp;
	switch (ctrls->loop_filter_mode) {
	case V4L2_MPEG_VIDEO_HEVC_LOOP_FILTER_MODE_DISABLED:
		output->en_dbk = 0;
		output->en_sao = 0;
		output->en_lf_cross_slice_boundary = 0;
		break;
	case V4L2_MPEG_VIDEO_HEVC_LOOP_FILTER_MODE_ENABLED:
		output->en_dbk = 1;
		output->en_sao = 1;
		output->en_lf_cross_slice_boundary = 1;
		break;
	case V4L2_MPEG_VIDEO_HEVC_LOOP_FILTER_MODE_DISABLED_AT_SLICE_BOUNDARY:
		output->en_dbk = 1;
		output->en_sao = 1;
		output->en_lf_cross_slice_boundary = 0;
		break;
	default:
		break;
	}
	switch (ctrls->refresh_type) {
	case V4L2_MPEG_VIDEO_HEVC_REFRESH_NONE:
		output->decoding_refresh_type = DEC_REFRESH_TYPE_NON_IRAP;
		break;
	case V4L2_MPEG_VIDEO_HEVC_REFRESH_IDR:
		output->decoding_refresh_type = DEC_REFRESH_TYPE_IDR;
		break;
	default:
		break;
	}
	output->intra_period = ctrls->refresh_period;
	if (output->idr_period) {
		output->decoding_refresh_type = DEC_REFRESH_TYPE_IDR;
		output->intra_period = output->idr_period;
		output->idr_period = 0;
	}
	output->beta_offset_div2 = ctrls->lf_beta_offset_div2;
	output->tc_offset_div2 = ctrls->lf_tc_offset_div2;
	output->en_constrained_intra_pred = ctrls->const_intra_pred;
	output->en_strong_intra_smoothing = ctrls->strong_smoothing;
	output->en_temporal_mvp = ctrls->tmv_prediction;
	output->num_ticks_poc_diff_one = DEFAULT_NUM_TICKS_POC_DIFF;
}

static void wave6_set_enc_open_param(struct enc_open_param *open_param,
				     struct vpu_instance *inst)
{
	struct enc_controls *ctrls = &inst->enc_ctrls;
	struct enc_wave_param *output = &open_param->wave_param;
	u32 ctu_size = (inst->std == W_AVC_ENC) ? 16 : 64;
	u32 num_ctu_row = ALIGN(inst->src_fmt.height, ctu_size) / ctu_size;

	open_param->source_endian = VPU_SOURCE_ENDIAN;
	if (inst->src_fmt.pixelformat == V4L2_PIX_FMT_YUV420 ||
	    inst->src_fmt.pixelformat == V4L2_PIX_FMT_NV12 ||
	    inst->src_fmt.pixelformat == V4L2_PIX_FMT_NV21 ||
	    inst->src_fmt.pixelformat == V4L2_PIX_FMT_YUV420M ||
	    inst->src_fmt.pixelformat == V4L2_PIX_FMT_NV12M ||
	    inst->src_fmt.pixelformat == V4L2_PIX_FMT_NV21M) {
		open_param->src_format = FORMAT_420;
	} else if (inst->src_fmt.pixelformat == V4L2_PIX_FMT_YUV422P ||
		   inst->src_fmt.pixelformat == V4L2_PIX_FMT_NV16 ||
		   inst->src_fmt.pixelformat == V4L2_PIX_FMT_NV61 ||
		   inst->src_fmt.pixelformat == V4L2_PIX_FMT_YUV422M ||
		   inst->src_fmt.pixelformat == V4L2_PIX_FMT_NV16M ||
		   inst->src_fmt.pixelformat == V4L2_PIX_FMT_NV61M) {
		open_param->src_format = FORMAT_422;
	} else if (inst->src_fmt.pixelformat == V4L2_PIX_FMT_NV24 ||
		   inst->src_fmt.pixelformat == V4L2_PIX_FMT_NV42) {
		open_param->src_format = FORMAT_YUV444_24BIT;
	} else if (inst->src_fmt.pixelformat == V4L2_PIX_FMT_YUV24) {
		open_param->src_format = FORMAT_YUV444_24BIT_PACKED;
	} else if (inst->src_fmt.pixelformat == V4L2_PIX_FMT_YUYV) {
		open_param->src_format = FORMAT_YUYV;
		open_param->packed_format = PACKED_YUYV;
	} else if (inst->src_fmt.pixelformat == V4L2_PIX_FMT_RGB24) {
		open_param->src_format = FORMAT_RGB_24BIT_PACKED;
	} else if (inst->src_fmt.pixelformat == V4L2_PIX_FMT_P010) {
		open_param->src_format = FORMAT_420_P10_16BIT_MSB;
		open_param->source_endian = VDI_128BIT_LE_BYTE_SWAP;
	} else if (inst->src_fmt.pixelformat == V4L2_PIX_FMT_ARGB32 ||
		   inst->src_fmt.pixelformat == V4L2_PIX_FMT_XRGB32 ||
		   inst->src_fmt.pixelformat == V4L2_PIX_FMT_RGBA32 ||
		   inst->src_fmt.pixelformat == V4L2_PIX_FMT_RGBX32) {
		open_param->src_format = FORMAT_RGB_32BIT_PACKED;
	} else if (inst->src_fmt.pixelformat == V4L2_PIX_FMT_ARGB2101010) {
		open_param->src_format = FORMAT_RGB_P10_32BIT_PACKED;
		open_param->source_endian = VDI_128BIT_LE_WORD_BYTE_SWAP;
	}
	open_param->line_buf_int_en = true;
	open_param->stream_endian = VPU_STREAM_ENDIAN;
	open_param->inst_buffer.temp_base = inst->dev->temp_vbuf.daddr;
	open_param->inst_buffer.temp_size = inst->dev->temp_vbuf.size;
	open_param->inst_buffer.ar_base = inst->ar_vbuf.daddr;
	open_param->pic_width = inst->dst_fmt.width;
	open_param->pic_height = inst->dst_fmt.height;

	output->custom_map_endian = VPU_USER_DATA_ENDIAN;
	output->gop_preset_idx = PRESET_IDX_IPP_SINGLE;
	output->temp_layer_cnt = 1;
	output->rc_initial_level = 8;
	output->pic_rc_max_dqp = 51;
	output->rc_initial_qp = -1;

	output->frame_rate = inst->frame_rate;
	output->idr_period = ctrls->gop_size;
	output->rc_mode = ctrls->bitrate_mode;
	output->rc_update_speed = (ctrls->bitrate_mode) ? ENC_RC_UPDATE_SPEED_CBR :
							  ENC_RC_UPDATE_SPEED_VBR;
	output->en_rate_control = ctrls->frame_rc_enable;
	output->en_cu_level_rate_control = ctrls->mb_rc_enable;
	output->max_intra_pic_bit = inst->dst_fmt.plane_fmt[0].sizeimage * 8;
	output->max_inter_pic_bit = inst->dst_fmt.plane_fmt[0].sizeimage * 8;
	output->enc_bit_rate = ctrls->bitrate;
	output->cpb_size = wave6_cpb_size_msec(ctrls->h264.cpb_size, ctrls->bitrate);
	output->slice_mode = ctrls->slice_mode;
	output->slice_arg = ctrls->slice_max_mb;
	output->forced_idr_header = ctrls->prepend_spspps_to_idr;
	if (ctrls->intra_refresh_period) {
		output->intra_refresh_mode = INTRA_REFRESH_ROW;
		// Calculate number of CTU rows based on number of frames.
		if (ctrls->intra_refresh_period < num_ctu_row) {
			output->intra_refresh_arg = (num_ctu_row + ctrls->intra_refresh_period - 1)
						    / ctrls->intra_refresh_period;
		} else {
			output->intra_refresh_arg = 1;
		}
	}
	output->sar.enable = ctrls->h264.vui_sar_enable;
	output->sar.idc = ctrls->h264.vui_sar_idc;
	if (output->sar.idc == V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_EXTENDED)
		output->sar.idc = H264_VUI_SAR_IDC_EXTENDED;
	output->sar.width = ctrls->h264.vui_ext_sar_width;
	output->sar.height = ctrls->h264.vui_ext_sar_height;
	output->color.video_signal_type_present_flag = 1;
	output->color.color_range = to_video_full_range_flag(inst->quantization);
	output->color.color_description_present_flag = 1;
	output->color.color_primaries = to_colour_primaries(inst->colorspace);
	output->color.transfer_characteristics = to_transfer_characteristics(inst->colorspace,
									     inst->xfer_func);
	output->color.matrix_coefficients = to_matrix_coeffs(inst->colorspace, inst->ycbcr_enc);

	switch (inst->std) {
	case W_AVC_ENC:
		wave6_set_enc_h264_param(output, &ctrls->h264);
		break;
	case W_HEVC_ENC:
		wave6_set_enc_hevc_param(output, &ctrls->hevc);
		break;
	case W_AV1_ENC:
		break;
	default:
		break;
	}
}

static int wave6_vpu_enc_create_instance(struct vpu_instance *inst)
{
	int ret;
	struct enc_open_param open_param;

	memset(&open_param, 0, sizeof(struct enc_open_param));

	wave6_vpu_activate(inst->dev);
	ret = pm_runtime_resume_and_get(inst->dev->dev);
	if (ret) {
		dev_err(inst->dev->dev, "runtime_resume failed %d\n", ret);
		return ret;
	}

	wave6_vpu_wait_activated(inst->dev);

	inst->std = wave6_to_vpu_wavestd(inst->dst_fmt.pixelformat);
	if (inst->std == STD_UNKNOWN) {
		dev_err(inst->dev->dev, "unsupported pixelformat: %.4s\n",
			(char *)&inst->dst_fmt.pixelformat);
		ret = -EINVAL;
		goto error_pm;
	}

	inst->ar_vbuf.size = ALIGN(WAVE6_ARBUF_SIZE, 4096);
	ret = wave6_alloc_dma(inst->dev->dev, &inst->ar_vbuf);
	if (ret) {
		dev_err(inst->dev->dev, "alloc ar of size %zu failed\n",
			inst->ar_vbuf.size);
		goto error_pm;
	}

	wave6_set_enc_open_param(&open_param, inst);

	ret = wave6_vpu_enc_open(inst, &open_param);
	if (ret) {
		dev_err(inst->dev->dev, "failed create instance : %d\n", ret);
		goto error_open;
	}

	dprintk(inst->dev->dev, "[%d] encoder\n", inst->id);
	wave6_vpu_create_dbgfs_file(inst);
	wave6_vpu_set_instance_state(inst, VPU_INST_STATE_OPEN);

	return 0;

error_open:
	wave6_free_dma(&inst->ar_vbuf);
error_pm:
	pm_runtime_put_sync(inst->dev->dev);
	return ret;
}

static int wave6_vpu_enc_initialize_instance(struct vpu_instance *inst)
{
	int ret;
	struct enc_initial_info initial_info;
	struct v4l2_ctrl *ctrl;

	if (inst->enc_ctrls.mirror_direction) {
		wave6_vpu_enc_give_command(inst, ENABLE_MIRRORING, NULL);
		wave6_vpu_enc_give_command(inst, SET_MIRROR_DIRECTION,
					   &inst->enc_ctrls.mirror_direction);
	}
	if (inst->enc_ctrls.rot_angle) {
		wave6_vpu_enc_give_command(inst, ENABLE_ROTATION, NULL);
		wave6_vpu_enc_give_command(inst, SET_ROTATION_ANGLE,
					   &inst->enc_ctrls.rot_angle);
	}

	ret = wave6_vpu_enc_issue_seq_init(inst);
	if (ret) {
		dev_err(inst->dev->dev, "seq init fail %d\n", ret);
		return ret;
	}

	if (wave6_vpu_wait_interrupt(inst, VPU_ENC_TIMEOUT) < 0) {
		dev_err(inst->dev->dev, "seq init timeout\n");
		return ret;
	}

	ret = wave6_vpu_enc_complete_seq_init(inst, &initial_info);
	if (ret) {
		dev_err(inst->dev->dev, "seq init error\n");
		return ret;
	}

	dev_dbg(inst->dev->dev, "min_fb_cnt : %d | min_src_cnt : %d\n",
		initial_info.min_frame_buffer_count,
		initial_info.min_src_frame_count);

	ctrl = v4l2_ctrl_find(&inst->v4l2_ctrl_hdl,
			      V4L2_CID_MIN_BUFFERS_FOR_OUTPUT);
	if (ctrl)
		v4l2_ctrl_s_ctrl(ctrl, initial_info.min_src_frame_count);

	wave6_vpu_set_instance_state(inst, VPU_INST_STATE_INIT_SEQ);

	return 0;
}

static int wave6_vpu_enc_prepare_fb(struct vpu_instance *inst)
{
	int ret;
	unsigned int i;
	unsigned int fb_num = 0;
	unsigned int mv_num = 0;
	unsigned int fb_stride = 0;
	unsigned int fb_height = 0;
	struct enc_info *p_enc_info = &inst->codec_info->enc_info;

	fb_num = p_enc_info->initial_info.min_frame_buffer_count;
	mv_num = p_enc_info->initial_info.req_mv_buffer_count;

	fb_stride = ALIGN(inst->dst_fmt.width, 32);
	fb_height = ALIGN(inst->dst_fmt.height, 32);

	for (i = 0; i < fb_num; i++) {
		struct frame_buffer *frame = &inst->frame_buf[i];
		struct vpu_buf *vframe = &inst->frame_vbuf[i];
		unsigned int l_size = fb_stride * fb_height;
		unsigned int ch_size = ALIGN(fb_stride / 2, 32) * fb_height;

		vframe->size = l_size + ch_size;
		ret = wave6_alloc_dma(inst->dev->dev, vframe);
		if (ret) {
			dev_err(inst->dev->dev, "alloc FBC buffer fail : %zu\n",
				vframe->size);
			goto error;
		}

		frame->buf_y = vframe->daddr;
		frame->buf_cb = vframe->daddr + l_size;
		frame->buf_cr = (dma_addr_t)-1;
		frame->stride = fb_stride;
		frame->height = fb_height;
		frame->map_type = COMPRESSED_FRAME_MAP;
	}

	ret = wave6_allocate_aux_buffer(inst, AUX_BUF_FBC_Y_TBL, fb_num);
	if (ret) {
		dev_err(inst->dev->dev, "alloc FBC_Y_TBL buffer fail\n");
		goto error;
	}
	ret = wave6_allocate_aux_buffer(inst, AUX_BUF_FBC_C_TBL, fb_num);
	if (ret) {
		dev_err(inst->dev->dev, "alloc FBC_C_TBL buffer fail\n");
		goto error;
	}
	ret = wave6_allocate_aux_buffer(inst, AUX_BUF_MV_COL, mv_num);
	if (ret) {
		dev_err(inst->dev->dev, "alloc MV_COL buffer fail\n");
		goto error;
	}
	ret = wave6_allocate_aux_buffer(inst, AUX_BUF_SUB_SAMPLE, fb_num);
	if (ret) {
		dev_err(inst->dev->dev, "alloc SUB_SAMPLE buffer fail\n");
		goto error;
	}
	if (inst->std == W_AV1_ENC) {
		ret = wave6_allocate_aux_buffer(inst, AUX_BUF_DEF_CDF, 1);
		if (ret) {
			dev_err(inst->dev->dev, "alloc DEF_CDF buffer fail\n");
			goto error;
		}
	}

	ret = wave6_vpu_enc_register_frame_buffer_ex(inst, fb_num, fb_stride,
						     fb_height,
						     COMPRESSED_FRAME_MAP);
	if (ret) {
		dev_err(inst->dev->dev, "register frame buffer fail %d\n", ret);
		goto error;
	}

	wave6_vpu_set_instance_state(inst, VPU_INST_STATE_PIC_RUN);

	return 0;

error:
	wave6_vpu_enc_release_fb(inst);
	return ret;
}

static int wave6_vpu_enc_queue_setup(struct vb2_queue *q, unsigned int *num_buffers,
				     unsigned int *num_planes, unsigned int sizes[],
				     struct device *alloc_devs[])
{
	struct vpu_instance *inst = vb2_get_drv_priv(q);
	struct v4l2_pix_format_mplane inst_format =
		(V4L2_TYPE_IS_OUTPUT(q->type)) ? inst->src_fmt : inst->dst_fmt;
	unsigned int i;

	dev_dbg(inst->dev->dev, "%s: num_buffers %d num_planes %d type %d\n",
		__func__, *num_buffers, *num_planes, q->type);

	if (*num_planes) {
		if (inst_format.num_planes != *num_planes)
			return -EINVAL;

		for (i = 0; i < *num_planes; i++) {
			if (sizes[i] < inst_format.plane_fmt[i].sizeimage)
				return -EINVAL;
		}
	} else {
		*num_planes = inst_format.num_planes;
		for (i = 0; i < *num_planes; i++) {
			sizes[i] = inst_format.plane_fmt[i].sizeimage;
			dev_dbg(inst->dev->dev, "size[%d] : %d\n", i, sizes[i]);
		}

		if (V4L2_TYPE_IS_OUTPUT(q->type)) {
			struct v4l2_ctrl *ctrl;
			unsigned int min_src_frame_count = 0;

			ctrl = v4l2_ctrl_find(&inst->v4l2_ctrl_hdl,
					      V4L2_CID_MIN_BUFFERS_FOR_OUTPUT);
			if (ctrl)
				min_src_frame_count = v4l2_ctrl_g_ctrl(ctrl);

			*num_buffers = max(*num_buffers, min_src_frame_count);
		}
	}

	return 0;
}

static void wave6_vpu_enc_buf_queue(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct vpu_instance *inst = vb2_get_drv_priv(vb->vb2_queue);
	struct vpu_buffer *vpu_buf = wave6_to_vpu_buf(vbuf);

	dev_dbg(inst->dev->dev, "type %4d index %4d size[0] %4ld size[1] : %4ld | size[2] : %4ld\n",
		vb->type, vb->index, vb2_plane_size(&vbuf->vb2_buf, 0),
		vb2_plane_size(&vbuf->vb2_buf, 1), vb2_plane_size(&vbuf->vb2_buf, 2));

	if (V4L2_TYPE_IS_OUTPUT(vb->type)) {
		vbuf->sequence = inst->queued_src_buf_num++;

		vpu_buf->ts_input = ktime_get_raw();
		vpu_buf->force_key_frame = inst->enc_ctrls.force_key_frame;
		inst->enc_ctrls.force_key_frame = false;
	} else {
		inst->queued_dst_buf_num++;
	}

	vpu_buf->consumed = false;
	vpu_buf->used = false;
	v4l2_m2m_buf_queue(inst->v4l2_fh.m2m_ctx, vbuf);
}

static void wave6_vpu_enc_buf_finish(struct vb2_buffer *vb)
{
	struct vpu_instance *inst = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct vpu_buffer *vpu_buf = wave6_to_vpu_buf(vbuf);
	struct v4l2_ctrl *ctrl;

	if (V4L2_TYPE_IS_OUTPUT(vb->type))
		return;

	ctrl = v4l2_ctrl_find(inst->v4l2_fh.ctrl_handler, V4L2_CID_MPEG_VIDEO_AVERAGE_QP);
	if (ctrl)
		v4l2_ctrl_s_ctrl(ctrl, vpu_buf->average_qp);
}

static int wave6_vpu_enc_start_streaming(struct vb2_queue *q, unsigned int count)
{
	struct vpu_instance *inst = vb2_get_drv_priv(q);
	struct v4l2_pix_format_mplane *fmt;
	struct vb2_queue *vq_peer;
	int ret = 0;

	if (V4L2_TYPE_IS_OUTPUT(q->type)) {
		fmt = &inst->src_fmt;
		vq_peer = v4l2_m2m_get_dst_vq(inst->v4l2_fh.m2m_ctx);
	} else {
		fmt = &inst->dst_fmt;
		vq_peer = v4l2_m2m_get_src_vq(inst->v4l2_fh.m2m_ctx);
	}

	dprintk(inst->dev->dev, "[%d] %s %c%c%c%c %dx%d, %d buffers\n",
		inst->id, V4L2_TYPE_IS_OUTPUT(q->type) ? "output" : "capture",
		fmt->pixelformat,
		fmt->pixelformat >> 8,
		fmt->pixelformat >> 16,
		fmt->pixelformat >> 24,
		fmt->width, fmt->height, vb2_get_num_buffers(q));

	if (!vb2_is_streaming(vq_peer))
		return 0;

	wave6_vpu_pause(inst->dev->dev, 0);

	if (inst->state == VPU_INST_STATE_NONE) {
		ret = wave6_vpu_enc_create_instance(inst);
		if (ret)
			goto exit;
	}

	if (inst->state == VPU_INST_STATE_OPEN) {
		ret = wave6_vpu_enc_initialize_instance(inst);
		if (ret) {
			wave6_vpu_enc_destroy_instance(inst);
			goto exit;
		}
	}

	if (inst->state == VPU_INST_STATE_INIT_SEQ) {
		ret = wave6_vpu_enc_prepare_fb(inst);
		if (ret) {
			wave6_vpu_enc_destroy_instance(inst);
			goto exit;
		}
	}

exit:
	wave6_vpu_pause(inst->dev->dev, 1);
	if (ret)
		wave6_vpu_return_buffers(inst, q->type, VB2_BUF_STATE_QUEUED);

	return ret;
}

static void wave6_vpu_enc_stop_streaming(struct vb2_queue *q)
{
	struct vpu_instance *inst = vb2_get_drv_priv(q);
	struct vb2_queue *vq_peer;

	dprintk(inst->dev->dev, "[%d] %s, input %d, decode %d\n",
		inst->id, V4L2_TYPE_IS_OUTPUT(q->type) ? "output" : "capture",
		inst->queued_src_buf_num, inst->sequence);

	if (inst->state == VPU_INST_STATE_NONE)
		goto exit;

	if (wave6_vpu_both_queues_are_streaming(inst))
		wave6_vpu_set_instance_state(inst, VPU_INST_STATE_STOP);

	wave6_vpu_pause(inst->dev->dev, 0);

	if (V4L2_TYPE_IS_OUTPUT(q->type)) {
		wave6_vpu_reset_performance(inst);
		inst->queued_src_buf_num = 0;
		inst->processed_buf_num = 0;
		inst->error_buf_num = 0;
		inst->sequence = 0;
		v4l2_m2m_set_src_buffered(inst->v4l2_fh.m2m_ctx, false);
	} else {
		inst->eos = false;
		inst->queued_dst_buf_num = 0;
	}

	if (V4L2_TYPE_IS_OUTPUT(q->type))
		vq_peer = v4l2_m2m_get_dst_vq(inst->v4l2_fh.m2m_ctx);
	else
		vq_peer = v4l2_m2m_get_src_vq(inst->v4l2_fh.m2m_ctx);

	if (!vb2_is_streaming(vq_peer) && inst->state != VPU_INST_STATE_NONE)
		wave6_vpu_enc_destroy_instance(inst);

	wave6_vpu_pause(inst->dev->dev, 1);

exit:
	wave6_vpu_return_buffers(inst, q->type, VB2_BUF_STATE_ERROR);
}

static const struct vb2_ops wave6_vpu_enc_vb2_ops = {
	.queue_setup = wave6_vpu_enc_queue_setup,
	.wait_prepare = vb2_ops_wait_prepare,
	.wait_finish = vb2_ops_wait_finish,
	.buf_queue = wave6_vpu_enc_buf_queue,
	.buf_finish = wave6_vpu_enc_buf_finish,
	.start_streaming = wave6_vpu_enc_start_streaming,
	.stop_streaming = wave6_vpu_enc_stop_streaming,
};

static void wave6_set_default_format(struct v4l2_pix_format_mplane *src_fmt,
				     struct v4l2_pix_format_mplane *dst_fmt)
{
	const struct vpu_format *vpu_fmt;

	vpu_fmt = wave6_find_vpu_fmt_by_idx(0, VPU_FMT_TYPE_RAW);
	if (vpu_fmt) {
		src_fmt->pixelformat = vpu_fmt->v4l2_pix_fmt;
		src_fmt->num_planes = vpu_fmt->num_planes;
		wave6_update_pix_fmt(src_fmt,
				     W6_DEF_ENC_PIC_WIDTH, W6_DEF_ENC_PIC_HEIGHT);
	}

	vpu_fmt = wave6_find_vpu_fmt_by_idx(0, VPU_FMT_TYPE_CODEC);
	if (vpu_fmt) {
		dst_fmt->pixelformat = vpu_fmt->v4l2_pix_fmt;
		dst_fmt->num_planes = vpu_fmt->num_planes;
		wave6_update_pix_fmt(dst_fmt,
				     W6_DEF_ENC_PIC_WIDTH, W6_DEF_ENC_PIC_HEIGHT);
	}
}

static int wave6_vpu_enc_queue_init(void *priv, struct vb2_queue *src_vq, struct vb2_queue *dst_vq)
{
	struct vpu_instance *inst = priv;
	int ret;

	src_vq->type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	src_vq->io_modes = VB2_MMAP | VB2_DMABUF;
	src_vq->mem_ops = &vb2_dma_contig_memops;
	src_vq->ops = &wave6_vpu_enc_vb2_ops;
	src_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	src_vq->buf_struct_size = sizeof(struct vpu_buffer);
	src_vq->drv_priv = inst;
	src_vq->lock = &inst->dev->dev_lock;
	src_vq->dev = inst->dev->v4l2_dev.dev;
	ret = vb2_queue_init(src_vq);
	if (ret)
		return ret;

	dst_vq->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	dst_vq->io_modes = VB2_MMAP | VB2_DMABUF;
	dst_vq->mem_ops = &vb2_dma_contig_memops;
	dst_vq->ops = &wave6_vpu_enc_vb2_ops;
	dst_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	dst_vq->buf_struct_size = sizeof(struct vpu_buffer);
	dst_vq->drv_priv = inst;
	dst_vq->lock = &inst->dev->dev_lock;
	dst_vq->dev = inst->dev->v4l2_dev.dev;
	ret = vb2_queue_init(dst_vq);
	if (ret)
		return ret;

	return 0;
}

static const struct vpu_instance_ops wave6_vpu_enc_inst_ops = {
	.start_process = wave6_vpu_enc_start_encode,
	.finish_process = wave6_vpu_enc_finish_encode,
};

static int wave6_vpu_open_enc(struct file *filp)
{
	struct video_device *vdev = video_devdata(filp);
	struct vpu_device *dev = video_drvdata(filp);
	struct vpu_instance *inst = NULL;
	struct v4l2_ctrl_handler *v4l2_ctrl_hdl;
	int ret;

	inst = kzalloc(sizeof(*inst), GFP_KERNEL);
	if (!inst)
		return -ENOMEM;
	v4l2_ctrl_hdl = &inst->v4l2_ctrl_hdl;

	inst->dev = dev;
	inst->type = VPU_INST_TYPE_ENC;
	inst->ops = &wave6_vpu_enc_inst_ops;

	v4l2_fh_init(&inst->v4l2_fh, vdev);
	filp->private_data = &inst->v4l2_fh;
	v4l2_fh_add(&inst->v4l2_fh);

	inst->v4l2_fh.m2m_ctx =
		v4l2_m2m_ctx_init(dev->m2m_dev, inst, wave6_vpu_enc_queue_init);
	if (IS_ERR(inst->v4l2_fh.m2m_ctx)) {
		ret = PTR_ERR(inst->v4l2_fh.m2m_ctx);
		goto free_inst;
	}

	v4l2_ctrl_handler_init(v4l2_ctrl_hdl, 50);
	v4l2_ctrl_new_std_menu(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			       V4L2_CID_MPEG_VIDEO_HEVC_PROFILE,
			       V4L2_MPEG_VIDEO_HEVC_PROFILE_MAIN, 0,
			       V4L2_MPEG_VIDEO_HEVC_PROFILE_MAIN);
	v4l2_ctrl_new_std_menu(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			       V4L2_CID_MPEG_VIDEO_HEVC_LEVEL,
			       V4L2_MPEG_VIDEO_HEVC_LEVEL_5_1, 0,
			       V4L2_MPEG_VIDEO_HEVC_LEVEL_5);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_HEVC_MIN_QP,
			  0, 51, 1, 8);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_HEVC_MAX_QP,
			  0, 51, 1, 51);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_HEVC_I_FRAME_QP,
			  0, 51, 1, 30);
	v4l2_ctrl_new_std_menu(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			       V4L2_CID_MPEG_VIDEO_HEVC_LOOP_FILTER_MODE,
			       V4L2_MPEG_VIDEO_HEVC_LOOP_FILTER_MODE_DISABLED_AT_SLICE_BOUNDARY, 0,
			       V4L2_MPEG_VIDEO_HEVC_LOOP_FILTER_MODE_DISABLED);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_HEVC_LF_BETA_OFFSET_DIV2,
			  -6, 6, 1, 0);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_HEVC_LF_TC_OFFSET_DIV2,
			  -6, 6, 1, 0);
	v4l2_ctrl_new_std_menu(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			       V4L2_CID_MPEG_VIDEO_HEVC_REFRESH_TYPE,
			       V4L2_MPEG_VIDEO_HEVC_REFRESH_IDR,
			       (1 << V4L2_MPEG_VIDEO_HEVC_REFRESH_CRA),
			       V4L2_MPEG_VIDEO_HEVC_REFRESH_IDR);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_HEVC_REFRESH_PERIOD,
			  0, 2047, 1, 0);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_HEVC_CONST_INTRA_PRED,
			  0, 1, 1, 0);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_HEVC_STRONG_SMOOTHING,
			  0, 1, 1, 0);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_HEVC_TMV_PREDICTION,
			  0, 1, 1, 0);
	v4l2_ctrl_new_std_menu(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			       V4L2_CID_MPEG_VIDEO_H264_PROFILE,
			       V4L2_MPEG_VIDEO_H264_PROFILE_HIGH, 0,
			       V4L2_MPEG_VIDEO_H264_PROFILE_BASELINE);
	v4l2_ctrl_new_std_menu(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			       V4L2_CID_MPEG_VIDEO_H264_LEVEL,
			       V4L2_MPEG_VIDEO_H264_LEVEL_5_2, 0,
			       V4L2_MPEG_VIDEO_H264_LEVEL_5_0);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_H264_MIN_QP,
			  0, 51, 1, 8);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_H264_MAX_QP,
			  0, 51, 1, 51);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_H264_I_FRAME_QP,
			  0, 51, 1, 30);
	v4l2_ctrl_new_std_menu(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			       V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_MODE,
			       V4L2_MPEG_VIDEO_H264_LOOP_FILTER_MODE_DISABLED_AT_SLICE_BOUNDARY, 0,
			       V4L2_MPEG_VIDEO_H264_LOOP_FILTER_MODE_DISABLED);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_ALPHA,
			  -6, 6, 1, 0);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_BETA,
			  -6, 6, 1, 0);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_H264_8X8_TRANSFORM,
			  0, 1, 1, 0);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_H264_CONSTRAINED_INTRA_PREDICTION,
			  0, 1, 1, 0);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_H264_CHROMA_QP_INDEX_OFFSET,
			  -12, 12, 1, 0);
	v4l2_ctrl_new_std_menu(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			       V4L2_CID_MPEG_VIDEO_H264_ENTROPY_MODE,
			       V4L2_MPEG_VIDEO_H264_ENTROPY_MODE_CABAC, 0,
			       V4L2_MPEG_VIDEO_H264_ENTROPY_MODE_CAVLC);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_H264_I_PERIOD,
			  0, 2047, 1, 0);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_H264_VUI_SAR_ENABLE, 0, 1, 1, 0);
	v4l2_ctrl_new_std_menu(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			       V4L2_CID_MPEG_VIDEO_H264_VUI_SAR_IDC,
			       V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_EXTENDED, 0,
			       V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_UNSPECIFIED);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_H264_VUI_EXT_SAR_WIDTH,
			  0, 0xFFFF, 1, 0);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_H264_VUI_EXT_SAR_HEIGHT,
			  0, 0xFFFF, 1, 0);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			  V4L2_CID_HFLIP,
			  0, 1, 1, 0);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			  V4L2_CID_VFLIP,
			  0, 1, 1, 0);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			  V4L2_CID_ROTATE,
			  0, 270, 90, 0);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_H264_CPB_SIZE,
			  0, 18750000, 1, 0);
	v4l2_ctrl_new_std_menu(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			       V4L2_CID_MPEG_VIDEO_BITRATE_MODE,
			       V4L2_MPEG_VIDEO_BITRATE_MODE_CBR, 0,
			       V4L2_MPEG_VIDEO_BITRATE_MODE_CBR);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_BITRATE,
			  1, 1500000000, 1, 2097152);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_FRAME_RC_ENABLE,
			  0, 1, 1, 1);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_MB_RC_ENABLE,
			  0, 1, 1, 1);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_GOP_SIZE,
			  0, 2047, 1, 30);
	v4l2_ctrl_new_std_menu(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			       V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MODE,
			       V4L2_MPEG_VIDEO_MULTI_SLICE_MODE_MAX_MB, 0,
			       V4L2_MPEG_VIDEO_MULTI_SLICE_MODE_SINGLE);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_MB,
			  0, 0x3FFFF, 1, 1);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_FORCE_KEY_FRAME,
			  0, 1, 1, 0);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_PREPEND_SPSPPS_TO_IDR,
			  0, 1, 1, 1);
	v4l2_ctrl_new_std_menu(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			       V4L2_CID_MPEG_VIDEO_INTRA_REFRESH_PERIOD_TYPE,
			       V4L2_CID_MPEG_VIDEO_INTRA_REFRESH_PERIOD_TYPE_CYCLIC,
			       (1 << V4L2_CID_MPEG_VIDEO_INTRA_REFRESH_PERIOD_TYPE_RANDOM),
			       V4L2_CID_MPEG_VIDEO_INTRA_REFRESH_PERIOD_TYPE_CYCLIC);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_INTRA_REFRESH_PERIOD,
			  0, 2160, 1, 0);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave6_vpu_enc_ctrl_ops,
			  V4L2_CID_MIN_BUFFERS_FOR_OUTPUT, 1, 32, 1, 1);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, NULL, V4L2_CID_MPEG_VIDEO_AVERAGE_QP, 0, 51, 1, 0);

	if (v4l2_ctrl_hdl->error) {
		ret = -ENODEV;
		goto err_m2m_release;
	}

	inst->v4l2_fh.ctrl_handler = v4l2_ctrl_hdl;
	v4l2_ctrl_handler_setup(v4l2_ctrl_hdl);

	wave6_set_default_format(&inst->src_fmt, &inst->dst_fmt);
	inst->crop.left = 0;
	inst->crop.top = 0;
	inst->crop.width = inst->dst_fmt.width;
	inst->crop.height = inst->dst_fmt.height;
	inst->colorspace = V4L2_COLORSPACE_DEFAULT;
	inst->ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
	inst->quantization = V4L2_QUANTIZATION_DEFAULT;
	inst->xfer_func = V4L2_XFER_FUNC_DEFAULT;
	inst->frame_rate = 30;

	return 0;

err_m2m_release:
	v4l2_m2m_ctx_release(inst->v4l2_fh.m2m_ctx);
free_inst:
	kfree(inst);
	return ret;
}

static int wave6_vpu_enc_release(struct file *filp)
{
	struct vpu_instance *inst = wave6_to_vpu_inst(filp->private_data);

	dprintk(inst->dev->dev, "[%d] release\n", inst->id);
	v4l2_m2m_ctx_release(inst->v4l2_fh.m2m_ctx);

	mutex_lock(&inst->dev->dev_lock);
	if (inst->state != VPU_INST_STATE_NONE) {
		wave6_vpu_pause(inst->dev->dev, 0);
		wave6_vpu_enc_destroy_instance(inst);
		wave6_vpu_pause(inst->dev->dev, 1);
	}
	mutex_unlock(&inst->dev->dev_lock);

	v4l2_ctrl_handler_free(&inst->v4l2_ctrl_hdl);
	v4l2_fh_del(&inst->v4l2_fh);
	v4l2_fh_exit(&inst->v4l2_fh);
	kfree(inst);

	return 0;
}

static const struct v4l2_file_operations wave6_vpu_enc_fops = {
	.owner = THIS_MODULE,
	.open = wave6_vpu_open_enc,
	.release = wave6_vpu_enc_release,
	.unlocked_ioctl = video_ioctl2,
	.poll = v4l2_m2m_fop_poll,
	.mmap = v4l2_m2m_fop_mmap,
};

int wave6_vpu_enc_register_device(struct vpu_device *dev)
{
	struct video_device *vdev_enc;
	int ret;

	vdev_enc = devm_kzalloc(dev->v4l2_dev.dev, sizeof(*vdev_enc), GFP_KERNEL);
	if (!vdev_enc)
		return -ENOMEM;

	dev->video_dev_enc = vdev_enc;

	strscpy(vdev_enc->name, VPU_ENC_DEV_NAME, sizeof(vdev_enc->name));
	vdev_enc->fops = &wave6_vpu_enc_fops;
	vdev_enc->ioctl_ops = &wave6_vpu_enc_ioctl_ops;
	vdev_enc->release = video_device_release_empty;
	vdev_enc->v4l2_dev = &dev->v4l2_dev;
	vdev_enc->vfl_dir = VFL_DIR_M2M;
	vdev_enc->device_caps = V4L2_CAP_VIDEO_M2M_MPLANE | V4L2_CAP_STREAMING;
	vdev_enc->lock = &dev->dev_lock;
	video_set_drvdata(vdev_enc, dev);

	ret = video_register_device(vdev_enc, VFL_TYPE_VIDEO, -1);
	if (ret)
		return ret;


	return 0;
}

void wave6_vpu_enc_unregister_device(struct vpu_device *dev)
{
	video_unregister_device(dev->video_dev_enc);
}
