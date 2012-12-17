/*
 * Copyright (C) 2012 Samsung Electronics Co.Ltd
<<<<<<< HEAD
 * Authors: YoungJun Cho <yj44.cho@samsung.com>
=======
 * Authors:
 *	YoungJun Cho <yj44.cho@samsung.com>
 *	Eunchul Kim <chulspro.kim@samsung.com>
>>>>>>> 3c2e81ef344a
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundationr
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>

<<<<<<< HEAD
#include "drmP.h"
#include "exynos_drm.h"
#include "exynos_drm_drv.h"
#include "exynos_drm_iommu.h"
#include "exynos_drm_ipp.h"

/* Configuration */
#define ROT_CONFIG			0x00
#define ROT_CONFIG_IRQ			(3 << 8)

/* Image Control */
#define ROT_CONTROL			0x10
#define ROT_CONTROL_PATTERN_WRITE	(1 << 16)
#define ROT_CONTROL_FMT_YCBCR420_2P	(1 << 8)
#define ROT_CONTROL_FMT_RGB888		(6 << 8)
#define ROT_CONTROL_FMT_MASK		(7 << 8)
#define ROT_CONTROL_FLIP_VERTICAL	(2 << 6)
#define ROT_CONTROL_FLIP_HORIZONTAL	(3 << 6)
#define ROT_CONTROL_FLIP_MASK		(3 << 6)
#define ROT_CONTROL_ROT_90		(1 << 4)
#define ROT_CONTROL_ROT_180		(2 << 4)
#define ROT_CONTROL_ROT_270		(3 << 4)
#define ROT_CONTROL_ROT_MASK		(3 << 4)
#define ROT_CONTROL_START		(1 << 0)

/* Status */
#define ROT_STATUS			0x20
#define ROT_STATUS_IRQ_PENDING(x)	(1 << (x))
#define ROT_STATUS_IRQ(x)		(((x) >> 8) & 0x3)
#define ROT_STATUS_IRQ_VAL_COMPLETE	1
#define ROT_STATUS_IRQ_VAL_ILLEGAL	2

/* Buffer Address */
#define ROT_SRC_BUF_ADDR(n)		(0x30 + ((n) << 2))
#define ROT_DST_BUF_ADDR(n)		(0x50 + ((n) << 2))

/* Buffer Size */
#define ROT_SRC_BUF_SIZE		0x3c
#define ROT_DST_BUF_SIZE		0x5c
#define ROT_SET_BUF_SIZE_H(x)		((x) << 16)
#define ROT_SET_BUF_SIZE_W(x)		((x) << 0)
#define ROT_GET_BUF_SIZE_H(x)		((x) >> 16)
#define ROT_GET_BUF_SIZE_W(x)		((x) & 0xffff)

/* Crop Position */
#define ROT_SRC_CROP_POS		0x40
#define ROT_DST_CROP_POS		0x60
#define ROT_CROP_POS_Y(x)		((x) << 16)
#define ROT_CROP_POS_X(x)		((x) << 0)

/* Source Crop Size */
#define ROT_SRC_CROP_SIZE		0x44
#define ROT_SRC_CROP_SIZE_H(x)		((x) << 16)
#define ROT_SRC_CROP_SIZE_W(x)		((x) << 0)

/* Round to nearest aligned value */
#define ROT_ALIGN(x, align, mask)	(((x) + (1 << ((align) - 1))) & (mask))
/* Minimum limit value */
#define ROT_MIN(min, mask)		(((min) + ~(mask)) & (mask))
/* Maximum limit value */
#define ROT_MAX(max, mask)		((max) & (mask))
=======
#include <drm/drmP.h>
#include <drm/exynos_drm.h>
#include "regs-rotator.h"
#include "exynos_drm.h"
#include "exynos_drm_ipp.h"

/*
 * Rotator supports image crop/rotator and input/output DMA operations.
 * input DMA reads image data from the memory.
 * output DMA writes image data to memory.
 *
 * M2M operation : supports crop/scale/rotation/csc so on.
 * Memory ----> Rotator H/W ----> Memory.
 */

/*
 * TODO
 * 1. check suspend/resume api if needed.
 * 2. need to check use case platform_device_id.
 * 3. check src/dst size with, height.
 * 4. need to add supported list in prop_list.
 */

#define get_rot_context(dev)	platform_get_drvdata(to_platform_device(dev))
#define get_ctx_from_ippdrv(ippdrv)	container_of(ippdrv,\
					struct rot_context, ippdrv);
#define rot_read(offset)		readl(rot->regs + (offset))
#define rot_write(cfg, offset)	writel(cfg, rot->regs + (offset))
>>>>>>> 3c2e81ef344a

enum rot_irq_status {
	ROT_IRQ_STATUS_COMPLETE	= 8,
	ROT_IRQ_STATUS_ILLEGAL	= 9,
};

<<<<<<< HEAD
=======
/*
 * A structure of limitation.
 *
 * @min_w: minimum width.
 * @min_h: minimum height.
 * @max_w: maximum width.
 * @max_h: maximum height.
 * @align: align size.
 */
>>>>>>> 3c2e81ef344a
struct rot_limit {
	u32	min_w;
	u32	min_h;
	u32	max_w;
	u32	max_h;
	u32	align;
};

<<<<<<< HEAD
=======
/*
 * A structure of limitation table.
 *
 * @ycbcr420_2p: case of YUV.
 * @rgb888: case of RGB.
 */
>>>>>>> 3c2e81ef344a
struct rot_limit_table {
	struct rot_limit	ycbcr420_2p;
	struct rot_limit	rgb888;
};

<<<<<<< HEAD
struct rot_context {
	struct rot_limit_table		*limit_tbl;
	struct clk			*clock;
	struct resource			*regs_res;
	void __iomem			*regs;
	int				irq;
	struct exynos_drm_ippdrv	ippdrv;
	int				cur_buf_id[EXYNOS_DRM_OPS_MAX];
	bool				suspended;
=======
/*
 * A structure of rotator context.
 * @ippdrv: prepare initialization using ippdrv.
 * @regs_res: register resources.
 * @regs: memory mapped io registers.
 * @clock: rotator gate clock.
 * @limit_tbl: limitation of rotator.
 * @irq: irq number.
 * @cur_buf_id: current operation buffer id.
 * @suspended: suspended state.
 */
struct rot_context {
	struct exynos_drm_ippdrv	ippdrv;
	struct resource	*regs_res;
	void __iomem	*regs;
	struct clk	*clock;
	struct rot_limit_table	*limit_tbl;
	int	irq;
	int	cur_buf_id[EXYNOS_DRM_OPS_MAX];
	bool	suspended;
>>>>>>> 3c2e81ef344a
};

static void rotator_reg_set_irq(struct rot_context *rot, bool enable)
{
<<<<<<< HEAD
	u32 value = readl(rot->regs + ROT_CONFIG);

	if (enable == true)
		value |= ROT_CONFIG_IRQ;
	else
		value &= ~ROT_CONFIG_IRQ;

	writel(value, rot->regs + ROT_CONFIG);
}

static u32 rotator_reg_get_format(struct rot_context *rot)
{
	u32 value = readl(rot->regs + ROT_CONTROL);
	value &= ROT_CONTROL_FMT_MASK;

	return value;
}

static void rotator_reg_set_format(struct rot_context *rot, u32 img_fmt)
{
	u32 value = readl(rot->regs + ROT_CONTROL);
	value &= ~ROT_CONTROL_FMT_MASK;

	switch (img_fmt) {
	case DRM_FORMAT_NV12:
	case DRM_FORMAT_NV12M:
		value |= ROT_CONTROL_FMT_YCBCR420_2P;
		break;
	case DRM_FORMAT_XRGB8888:
		value |= ROT_CONTROL_FMT_RGB888;
		break;
	default:
		DRM_ERROR("invalid image format\n");
		return;
	}

	writel(value, rot->regs + ROT_CONTROL);
}

static void rotator_reg_set_flip(struct rot_context *rot,
						enum drm_exynos_flip flip)
{
	u32 value = readl(rot->regs + ROT_CONTROL);
	value &= ~ROT_CONTROL_FLIP_MASK;

	switch (flip) {
	case EXYNOS_DRM_FLIP_VERTICAL:
		value |= ROT_CONTROL_FLIP_VERTICAL;
		break;
	case EXYNOS_DRM_FLIP_HORIZONTAL:
		value |= ROT_CONTROL_FLIP_HORIZONTAL;
		break;
	default:
		/* Flip None */
		break;
	}

	writel(value, rot->regs + ROT_CONTROL);
}

static void rotator_reg_set_rotation(struct rot_context *rot,
					enum drm_exynos_degree degree)
{
	u32 value = readl(rot->regs + ROT_CONTROL);
	value &= ~ROT_CONTROL_ROT_MASK;

	switch (degree) {
	case EXYNOS_DRM_DEGREE_90:
		value |= ROT_CONTROL_ROT_90;
		break;
	case EXYNOS_DRM_DEGREE_180:
		value |= ROT_CONTROL_ROT_180;
		break;
	case EXYNOS_DRM_DEGREE_270:
		value |= ROT_CONTROL_ROT_270;
		break;
	default:
		/* Rotation 0 Degree */
		break;
	}

	writel(value, rot->regs + ROT_CONTROL);
}

static void rotator_reg_set_start(struct rot_context *rot)
{
	u32 value = readl(rot->regs + ROT_CONTROL);

	value |= ROT_CONTROL_START;

	writel(value, rot->regs + ROT_CONTROL);
=======
	u32 val = rot_read(ROT_CONFIG);

	if (enable == true)
		val |= ROT_CONFIG_IRQ;
	else
		val &= ~ROT_CONFIG_IRQ;

	rot_write(val, ROT_CONFIG);
}

static u32 rotator_reg_get_fmt(struct rot_context *rot)
{
	u32 val = rot_read(ROT_CONTROL);

	val &= ROT_CONTROL_FMT_MASK;

	return val;
>>>>>>> 3c2e81ef344a
}

static enum rot_irq_status rotator_reg_get_irq_status(struct rot_context *rot)
{
<<<<<<< HEAD
	u32 value = readl(rot->regs + ROT_STATUS);
	value = ROT_STATUS_IRQ(value);

	if (value == ROT_STATUS_IRQ_VAL_COMPLETE)
		return ROT_IRQ_STATUS_COMPLETE;
	else
		return ROT_IRQ_STATUS_ILLEGAL;
}

static void rotator_reg_set_irq_status_clear(struct rot_context *rot,
						enum rot_irq_status status)
{
	u32 value = readl(rot->regs + ROT_STATUS);

	value |= ROT_STATUS_IRQ_PENDING((u32)status);

	writel(value, rot->regs + ROT_STATUS);
}

static void rotator_reg_set_src_buf_addr(struct rot_context *rot,
							dma_addr_t addr, int i)
{
	writel(addr, rot->regs + ROT_SRC_BUF_ADDR(i));
}

static void rotator_reg_get_src_buf_size(struct rot_context *rot, u32 *w,
									u32 *h)
{
	u32 value = readl(rot->regs + ROT_SRC_BUF_SIZE);

	*w = ROT_GET_BUF_SIZE_W(value);
	*h = ROT_GET_BUF_SIZE_H(value);
}

static void rotator_reg_set_src_buf_size(struct rot_context *rot, u32 w, u32 h)
{
	u32 value = ROT_SET_BUF_SIZE_H(h) | ROT_SET_BUF_SIZE_W(w);

	writel(value, rot->regs + ROT_SRC_BUF_SIZE);
}

static void rotator_reg_set_src_crop_pos(struct rot_context *rot, u32 x, u32 y)
{
	u32 value = ROT_CROP_POS_Y(y) | ROT_CROP_POS_X(x);

	writel(value, rot->regs + ROT_SRC_CROP_POS);
}

static void rotator_reg_set_src_crop_size(struct rot_context *rot, u32 w, u32 h)
{
	u32 value = ROT_SRC_CROP_SIZE_H(h) | ROT_SRC_CROP_SIZE_W(w);

	writel(value, rot->regs + ROT_SRC_CROP_SIZE);
}

static void rotator_reg_set_dst_buf_addr(struct rot_context *rot,
							dma_addr_t addr, int i)
{
	writel(addr, rot->regs + ROT_DST_BUF_ADDR(i));
}

static void rotator_reg_get_dst_buf_size(struct rot_context *rot, u32 *w,
									u32 *h)
{
	u32 value = readl(rot->regs + ROT_DST_BUF_SIZE);

	*w = ROT_GET_BUF_SIZE_W(value);
	*h = ROT_GET_BUF_SIZE_H(value);
}

static void rotator_reg_set_dst_buf_size(struct rot_context *rot, u32 w, u32 h)
{
	u32 value = ROT_SET_BUF_SIZE_H(h) | ROT_SET_BUF_SIZE_W(w);

	writel(value, rot->regs + ROT_DST_BUF_SIZE);
}

static void rotator_reg_set_dst_crop_pos(struct rot_context *rot, u32 x, u32 y)
{
	u32 value = ROT_CROP_POS_Y(y) | ROT_CROP_POS_X(x);

	writel(value, rot->regs + ROT_DST_CROP_POS);
}

static void rotator_reg_get_dump(struct rot_context *rot)
{
	u32 value, i;

	for (i = 0; i <= ROT_DST_CROP_POS; i += 0x4) {
		value = readl(rot->regs + i);
		DRM_INFO("[%s] [0x%x] : 0x%x\n", __func__, i, value);
	}
=======
	u32 val = rot_read(ROT_STATUS);

	val = ROT_STATUS_IRQ(val);

	if (val == ROT_STATUS_IRQ_VAL_COMPLETE)
		return ROT_IRQ_STATUS_COMPLETE;

	return ROT_IRQ_STATUS_ILLEGAL;
>>>>>>> 3c2e81ef344a
}

static irqreturn_t rotator_irq_handler(int irq, void *arg)
{
	struct rot_context *rot = arg;
	struct exynos_drm_ippdrv *ippdrv = &rot->ippdrv;
<<<<<<< HEAD
	enum rot_irq_status irq_status;

	/* Get execution result */
	irq_status = rotator_reg_get_irq_status(rot);
	rotator_reg_set_irq_status_clear(rot, irq_status);

	if (irq_status == ROT_IRQ_STATUS_COMPLETE)
		ipp_send_event_handler(ippdrv,
					rot->cur_buf_id[EXYNOS_DRM_OPS_DST]);
	else {
		DRM_ERROR("the SFR is set illegally\n");
		rotator_reg_get_dump(rot);
	}
=======
	struct drm_exynos_ipp_cmd_node *c_node = ippdrv->cmd;
	struct drm_exynos_ipp_event_work *event_work = c_node->event_work;
	enum rot_irq_status irq_status;
	u32 val;

	/* Get execution result */
	irq_status = rotator_reg_get_irq_status(rot);

	/* clear status */
	val = rot_read(ROT_STATUS);
	val |= ROT_STATUS_IRQ_PENDING((u32)irq_status);
	rot_write(val, ROT_STATUS);

	if (irq_status == ROT_IRQ_STATUS_COMPLETE) {
		event_work->ippdrv = ippdrv;
		event_work->buf_id[EXYNOS_DRM_OPS_DST] =
			rot->cur_buf_id[EXYNOS_DRM_OPS_DST];
		queue_work(ippdrv->event_workq,
			(struct work_struct *)event_work);
	} else
		DRM_ERROR("the SFR is set illegally\n");
>>>>>>> 3c2e81ef344a

	return IRQ_HANDLED;
}

static void rotator_align_size(struct rot_context *rot, u32 fmt, u32 *hsize,
<<<<<<< HEAD
								u32 *vsize)
{
	struct rot_limit_table *limit_tbl = rot->limit_tbl;
	struct rot_limit *limit;
	u32 mask, value;
=======
		u32 *vsize)
{
	struct rot_limit_table *limit_tbl = rot->limit_tbl;
	struct rot_limit *limit;
	u32 mask, val;
>>>>>>> 3c2e81ef344a

	/* Get size limit */
	if (fmt == ROT_CONTROL_FMT_RGB888)
		limit = &limit_tbl->rgb888;
	else
		limit = &limit_tbl->ycbcr420_2p;

<<<<<<< HEAD
	/* Get mask for rounding to nearest aligned value */
	mask = ~((1 << limit->align) - 1);

	/* Set aligned width */
	value = ROT_ALIGN(*hsize, limit->align, mask);
	if (value < limit->min_w)
		*hsize = ROT_MIN(limit->min_w, mask);
	else if (value > limit->max_w)
		*hsize = ROT_MAX(limit->max_w, mask);
	else
		*hsize = value;

	/* Set aligned height */
	value = ROT_ALIGN(*vsize, limit->align, mask);
	if (value < limit->min_h)
		*vsize = ROT_MIN(limit->min_h, mask);
	else if (value > limit->max_h)
		*vsize = ROT_MAX(limit->max_h, mask);
	else
		*vsize = value;
=======
	/* Get mask for rounding to nearest aligned val */
	mask = ~((1 << limit->align) - 1);

	/* Set aligned width */
	val = ROT_ALIGN(*hsize, limit->align, mask);
	if (val < limit->min_w)
		*hsize = ROT_MIN(limit->min_w, mask);
	else if (val > limit->max_w)
		*hsize = ROT_MAX(limit->max_w, mask);
	else
		*hsize = val;

	/* Set aligned height */
	val = ROT_ALIGN(*vsize, limit->align, mask);
	if (val < limit->min_h)
		*vsize = ROT_MIN(limit->min_h, mask);
	else if (val > limit->max_h)
		*vsize = ROT_MAX(limit->max_h, mask);
	else
		*vsize = val;
>>>>>>> 3c2e81ef344a
}

static int rotator_src_set_fmt(struct device *dev, u32 fmt)
{
	struct rot_context *rot = dev_get_drvdata(dev);
<<<<<<< HEAD

	/* Set format configuration */
	rotator_reg_set_format(rot, fmt);
=======
	u32 val;

	val = rot_read(ROT_CONTROL);
	val &= ~ROT_CONTROL_FMT_MASK;

	switch (fmt) {
	case DRM_FORMAT_NV12:
		val |= ROT_CONTROL_FMT_YCBCR420_2P;
		break;
	case DRM_FORMAT_XRGB8888:
		val |= ROT_CONTROL_FMT_RGB888;
		break;
	default:
		DRM_ERROR("invalid image format\n");
		return -EINVAL;
	}

	rot_write(val, ROT_CONTROL);
>>>>>>> 3c2e81ef344a

	return 0;
}

<<<<<<< HEAD
static int rotator_src_set_size(struct device *dev, int swap,
						struct drm_exynos_pos *pos,
						struct drm_exynos_sz *sz)
{
	struct rot_context *rot = dev_get_drvdata(dev);
	u32 fmt, hsize, vsize;

	/* Get format */
	fmt = rotator_reg_get_format(rot);
=======
static inline bool rotator_check_reg_fmt(u32 fmt)
{
	if ((fmt == ROT_CONTROL_FMT_YCBCR420_2P) ||
	    (fmt == ROT_CONTROL_FMT_RGB888))
		return true;

	return false;
}

static int rotator_src_set_size(struct device *dev, int swap,
		struct drm_exynos_pos *pos,
		struct drm_exynos_sz *sz)
{
	struct rot_context *rot = dev_get_drvdata(dev);
	u32 fmt, hsize, vsize;
	u32 val;

	/* Get format */
	fmt = rotator_reg_get_fmt(rot);
	if (!rotator_check_reg_fmt(fmt)) {
		DRM_ERROR("%s:invalid format.\n", __func__);
		return -EINVAL;
	}
>>>>>>> 3c2e81ef344a

	/* Align buffer size */
	hsize = sz->hsize;
	vsize = sz->vsize;
	rotator_align_size(rot, fmt, &hsize, &vsize);

	/* Set buffer size configuration */
<<<<<<< HEAD
	rotator_reg_set_src_buf_size(rot, hsize, vsize);

	/* Set crop image position configuration */
	rotator_reg_set_src_crop_pos(rot, pos->x, pos->y);
	rotator_reg_set_src_crop_size(rot, pos->w, pos->h);
=======
	val = ROT_SET_BUF_SIZE_H(vsize) | ROT_SET_BUF_SIZE_W(hsize);
	rot_write(val, ROT_SRC_BUF_SIZE);

	/* Set crop image position configuration */
	val = ROT_CROP_POS_Y(pos->y) | ROT_CROP_POS_X(pos->x);
	rot_write(val, ROT_SRC_CROP_POS);
	val = ROT_SRC_CROP_SIZE_H(pos->h) | ROT_SRC_CROP_SIZE_W(pos->w);
	rot_write(val, ROT_SRC_CROP_SIZE);
>>>>>>> 3c2e81ef344a

	return 0;
}

static int rotator_src_set_addr(struct device *dev,
<<<<<<< HEAD
				struct drm_exynos_ipp_buf_info *buf_info,
				u32 buf_id, enum drm_exynos_ipp_buf_ctrl ctrl)
{
	struct rot_context *rot = dev_get_drvdata(dev);
	dma_addr_t addr[EXYNOS_DRM_PLANAR_MAX];
	u32 fmt, hsize, vsize;
=======
		struct drm_exynos_ipp_buf_info *buf_info,
		u32 buf_id, enum drm_exynos_ipp_buf_type buf_type)
{
	struct rot_context *rot = dev_get_drvdata(dev);
	dma_addr_t addr[EXYNOS_DRM_PLANAR_MAX];
	u32 val, fmt, hsize, vsize;
>>>>>>> 3c2e81ef344a
	int i;

	/* Set current buf_id */
	rot->cur_buf_id[EXYNOS_DRM_OPS_SRC] = buf_id;

<<<<<<< HEAD
	switch (ctrl) {
	case IPP_BUF_CTRL_QUEUE:
		/* Set address configuration */
		for (i = 0; i < EXYNOS_DRM_PLANAR_MAX; i++)
			addr[i] = buf_info->base[i];

		/* Get format */
		fmt = rotator_reg_get_format(rot);

		/* Re-set cb planar for NV12 format */
		if ((fmt == ROT_CONTROL_FMT_YCBCR420_2P) &&
					(addr[EXYNOS_DRM_PLANAR_CB] == 0x00)) {
			/* Get buf size */
			rotator_reg_get_src_buf_size(rot, &hsize, &vsize);
=======
	switch (buf_type) {
	case IPP_BUF_ENQUEUE:
		/* Set address configuration */
		for_each_ipp_planar(i)
			addr[i] = buf_info->base[i];

		/* Get format */
		fmt = rotator_reg_get_fmt(rot);
		if (!rotator_check_reg_fmt(fmt)) {
			DRM_ERROR("%s:invalid format.\n", __func__);
			return -EINVAL;
		}

		/* Re-set cb planar for NV12 format */
		if ((fmt == ROT_CONTROL_FMT_YCBCR420_2P) &&
		    !addr[EXYNOS_DRM_PLANAR_CB]) {

			val = rot_read(ROT_SRC_BUF_SIZE);
			hsize = ROT_GET_BUF_SIZE_W(val);
			vsize = ROT_GET_BUF_SIZE_H(val);
>>>>>>> 3c2e81ef344a

			/* Set cb planar */
			addr[EXYNOS_DRM_PLANAR_CB] =
				addr[EXYNOS_DRM_PLANAR_Y] + hsize * vsize;
		}

<<<<<<< HEAD
		for (i = 0; i < EXYNOS_DRM_PLANAR_MAX; i++)
			rotator_reg_set_src_buf_addr(rot, addr[i], i);
		break;
	case IPP_BUF_CTRL_DEQUEUE:
		for (i = 0; i < EXYNOS_DRM_PLANAR_MAX; i++)
			rotator_reg_set_src_buf_addr(rot, buf_info->base[i], i);
=======
		for_each_ipp_planar(i)
			rot_write(addr[i], ROT_SRC_BUF_ADDR(i));
		break;
	case IPP_BUF_DEQUEUE:
		for_each_ipp_planar(i)
			rot_write(0x0, ROT_SRC_BUF_ADDR(i));
>>>>>>> 3c2e81ef344a
		break;
	default:
		/* Nothing to do */
		break;
	}

	return 0;
}

static int rotator_dst_set_transf(struct device *dev,
<<<<<<< HEAD
					enum drm_exynos_degree degree,
					enum drm_exynos_flip flip)
{
	struct rot_context *rot = dev_get_drvdata(dev);

	/* Set transform configuration */
	rotator_reg_set_flip(rot, flip);
	rotator_reg_set_rotation(rot, degree);

	/* Check degree for setting buffer size swap */
	if ((degree == EXYNOS_DRM_DEGREE_90) ||
		(degree == EXYNOS_DRM_DEGREE_270))
		return 1;
	else
		return 0;
}

static int rotator_dst_set_size(struct device *dev, int swap,
						struct drm_exynos_pos *pos,
						struct drm_exynos_sz *sz)
{
	struct rot_context *rot = dev_get_drvdata(dev);
	u32 fmt, hsize, vsize;

	/* Get format */
	fmt = rotator_reg_get_format(rot);
=======
		enum drm_exynos_degree degree,
		enum drm_exynos_flip flip, bool *swap)
{
	struct rot_context *rot = dev_get_drvdata(dev);
	u32 val;

	/* Set transform configuration */
	val = rot_read(ROT_CONTROL);
	val &= ~ROT_CONTROL_FLIP_MASK;

	switch (flip) {
	case EXYNOS_DRM_FLIP_VERTICAL:
		val |= ROT_CONTROL_FLIP_VERTICAL;
		break;
	case EXYNOS_DRM_FLIP_HORIZONTAL:
		val |= ROT_CONTROL_FLIP_HORIZONTAL;
		break;
	default:
		/* Flip None */
		break;
	}

	val &= ~ROT_CONTROL_ROT_MASK;

	switch (degree) {
	case EXYNOS_DRM_DEGREE_90:
		val |= ROT_CONTROL_ROT_90;
		break;
	case EXYNOS_DRM_DEGREE_180:
		val |= ROT_CONTROL_ROT_180;
		break;
	case EXYNOS_DRM_DEGREE_270:
		val |= ROT_CONTROL_ROT_270;
		break;
	default:
		/* Rotation 0 Degree */
		break;
	}

	rot_write(val, ROT_CONTROL);

	/* Check degree for setting buffer size swap */
	if ((degree == EXYNOS_DRM_DEGREE_90) ||
	    (degree == EXYNOS_DRM_DEGREE_270))
		*swap = true;
	else
		*swap = false;

	return 0;
}

static int rotator_dst_set_size(struct device *dev, int swap,
		struct drm_exynos_pos *pos,
		struct drm_exynos_sz *sz)
{
	struct rot_context *rot = dev_get_drvdata(dev);
	u32 val, fmt, hsize, vsize;

	/* Get format */
	fmt = rotator_reg_get_fmt(rot);
	if (!rotator_check_reg_fmt(fmt)) {
		DRM_ERROR("%s:invalid format.\n", __func__);
		return -EINVAL;
	}
>>>>>>> 3c2e81ef344a

	/* Align buffer size */
	hsize = sz->hsize;
	vsize = sz->vsize;
	rotator_align_size(rot, fmt, &hsize, &vsize);

	/* Set buffer size configuration */
<<<<<<< HEAD
	rotator_reg_set_dst_buf_size(rot, hsize, vsize);

	/* Set crop image position configuration */
	rotator_reg_set_dst_crop_pos(rot, pos->x, pos->y);
=======
	val = ROT_SET_BUF_SIZE_H(vsize) | ROT_SET_BUF_SIZE_W(hsize);
	rot_write(val, ROT_DST_BUF_SIZE);

	/* Set crop image position configuration */
	val = ROT_CROP_POS_Y(pos->y) | ROT_CROP_POS_X(pos->x);
	rot_write(val, ROT_DST_CROP_POS);
>>>>>>> 3c2e81ef344a

	return 0;
}

static int rotator_dst_set_addr(struct device *dev,
<<<<<<< HEAD
				struct drm_exynos_ipp_buf_info *buf_info,
				u32 buf_id, enum drm_exynos_ipp_buf_ctrl ctrl)
{
	struct rot_context *rot = dev_get_drvdata(dev);
	dma_addr_t addr[EXYNOS_DRM_PLANAR_MAX];
	u32 fmt, hsize, vsize;
=======
		struct drm_exynos_ipp_buf_info *buf_info,
		u32 buf_id, enum drm_exynos_ipp_buf_type buf_type)
{
	struct rot_context *rot = dev_get_drvdata(dev);
	dma_addr_t addr[EXYNOS_DRM_PLANAR_MAX];
	u32 val, fmt, hsize, vsize;
>>>>>>> 3c2e81ef344a
	int i;

	/* Set current buf_id */
	rot->cur_buf_id[EXYNOS_DRM_OPS_DST] = buf_id;

<<<<<<< HEAD
	switch (ctrl) {
	case IPP_BUF_CTRL_QUEUE:
		/* Set address configuration */
		for (i = 0; i < EXYNOS_DRM_PLANAR_MAX; i++)
			addr[i] = buf_info->base[i];

		/* Get format */
		fmt = rotator_reg_get_format(rot);

		/* Re-set cb planar for NV12 format */
		if ((fmt == ROT_CONTROL_FMT_YCBCR420_2P) &&
					(addr[EXYNOS_DRM_PLANAR_CB] == 0x00)) {
			/* Get buf size */
			rotator_reg_get_dst_buf_size(rot, &hsize, &vsize);
=======
	switch (buf_type) {
	case IPP_BUF_ENQUEUE:
		/* Set address configuration */
		for_each_ipp_planar(i)
			addr[i] = buf_info->base[i];

		/* Get format */
		fmt = rotator_reg_get_fmt(rot);
		if (!rotator_check_reg_fmt(fmt)) {
			DRM_ERROR("%s:invalid format.\n", __func__);
			return -EINVAL;
		}

		/* Re-set cb planar for NV12 format */
		if ((fmt == ROT_CONTROL_FMT_YCBCR420_2P) &&
		    !addr[EXYNOS_DRM_PLANAR_CB]) {
			/* Get buf size */
			val = rot_read(ROT_DST_BUF_SIZE);

			hsize = ROT_GET_BUF_SIZE_W(val);
			vsize = ROT_GET_BUF_SIZE_H(val);
>>>>>>> 3c2e81ef344a

			/* Set cb planar */
			addr[EXYNOS_DRM_PLANAR_CB] =
				addr[EXYNOS_DRM_PLANAR_Y] + hsize * vsize;
		}

<<<<<<< HEAD
		for (i = 0; i < EXYNOS_DRM_PLANAR_MAX; i++)
			rotator_reg_set_dst_buf_addr(rot, addr[i], i);
		break;
	case IPP_BUF_CTRL_DEQUEUE:
		for (i = 0; i < EXYNOS_DRM_PLANAR_MAX; i++)
			rotator_reg_set_dst_buf_addr(rot, buf_info->base[i], i);
=======
		for_each_ipp_planar(i)
			rot_write(addr[i], ROT_DST_BUF_ADDR(i));
		break;
	case IPP_BUF_DEQUEUE:
		for_each_ipp_planar(i)
			rot_write(0x0, ROT_DST_BUF_ADDR(i));
>>>>>>> 3c2e81ef344a
		break;
	default:
		/* Nothing to do */
		break;
	}

	return 0;
}

static struct exynos_drm_ipp_ops rot_src_ops = {
	.set_fmt	=	rotator_src_set_fmt,
	.set_size	=	rotator_src_set_size,
	.set_addr	=	rotator_src_set_addr,
};

static struct exynos_drm_ipp_ops rot_dst_ops = {
	.set_transf	=	rotator_dst_set_transf,
	.set_size	=	rotator_dst_set_size,
	.set_addr	=	rotator_dst_set_addr,
};

<<<<<<< HEAD
static int rotator_ippdrv_check_property(struct device *dev,
				struct drm_exynos_ipp_property *property)
=======
static int rotator_init_prop_list(struct exynos_drm_ippdrv *ippdrv)
{
	struct drm_exynos_ipp_prop_list *prop_list;

	DRM_DEBUG_KMS("%s\n", __func__);

	prop_list = devm_kzalloc(ippdrv->dev, sizeof(*prop_list), GFP_KERNEL);
	if (!prop_list) {
		DRM_ERROR("failed to alloc property list.\n");
		return -ENOMEM;
	}

	prop_list->version = 1;
	prop_list->flip = (1 << EXYNOS_DRM_FLIP_VERTICAL) |
				(1 << EXYNOS_DRM_FLIP_HORIZONTAL);
	prop_list->degree = (1 << EXYNOS_DRM_DEGREE_0) |
				(1 << EXYNOS_DRM_DEGREE_90) |
				(1 << EXYNOS_DRM_DEGREE_180) |
				(1 << EXYNOS_DRM_DEGREE_270);
	prop_list->csc = 0;
	prop_list->crop = 0;
	prop_list->scale = 0;

	ippdrv->prop_list = prop_list;

	return 0;
}

static inline bool rotator_check_drm_fmt(u32 fmt)
{
	switch (fmt) {
	case DRM_FORMAT_XRGB8888:
	case DRM_FORMAT_NV12:
		return true;
	default:
		DRM_DEBUG_KMS("%s:not support format\n", __func__);
		return false;
	}
}

static inline bool rotator_check_drm_flip(enum drm_exynos_flip flip)
{
	switch (flip) {
	case EXYNOS_DRM_FLIP_NONE:
	case EXYNOS_DRM_FLIP_VERTICAL:
	case EXYNOS_DRM_FLIP_HORIZONTAL:
		return true;
	default:
		DRM_DEBUG_KMS("%s:invalid flip\n", __func__);
		return false;
	}
}

static int rotator_ippdrv_check_property(struct device *dev,
		struct drm_exynos_ipp_property *property)
>>>>>>> 3c2e81ef344a
{
	struct drm_exynos_ipp_config *src_config =
					&property->config[EXYNOS_DRM_OPS_SRC];
	struct drm_exynos_ipp_config *dst_config =
					&property->config[EXYNOS_DRM_OPS_DST];
	struct drm_exynos_pos *src_pos = &src_config->pos;
	struct drm_exynos_pos *dst_pos = &dst_config->pos;
	struct drm_exynos_sz *src_sz = &src_config->sz;
	struct drm_exynos_sz *dst_sz = &dst_config->sz;
	bool swap = false;

	/* Check format configuration */
	if (src_config->fmt != dst_config->fmt) {
<<<<<<< HEAD
		DRM_DEBUG_KMS("[%s]not support csc feature\n", __func__);
		return -EINVAL;
	}

	switch (src_config->fmt) {
	case DRM_FORMAT_XRGB8888:
	case DRM_FORMAT_NV12:
	case DRM_FORMAT_NV12M:
		/* No problem */
		break;
	default:
		DRM_DEBUG_KMS("[%s]not support format\n", __func__);
=======
		DRM_DEBUG_KMS("%s:not support csc feature\n", __func__);
		return -EINVAL;
	}

	if (!rotator_check_drm_fmt(dst_config->fmt)) {
		DRM_DEBUG_KMS("%s:invalid format\n", __func__);
>>>>>>> 3c2e81ef344a
		return -EINVAL;
	}

	/* Check transform configuration */
	if (src_config->degree != EXYNOS_DRM_DEGREE_0) {
<<<<<<< HEAD
		DRM_DEBUG_KMS("[%s]not support source-side rotation\n",
								__func__);
=======
		DRM_DEBUG_KMS("%s:not support source-side rotation\n",
			__func__);
>>>>>>> 3c2e81ef344a
		return -EINVAL;
	}

	switch (dst_config->degree) {
	case EXYNOS_DRM_DEGREE_90:
	case EXYNOS_DRM_DEGREE_270:
		swap = true;
	case EXYNOS_DRM_DEGREE_0:
	case EXYNOS_DRM_DEGREE_180:
		/* No problem */
		break;
	default:
<<<<<<< HEAD
		DRM_DEBUG_KMS("[%s]invalid degree\n", __func__);
=======
		DRM_DEBUG_KMS("%s:invalid degree\n", __func__);
>>>>>>> 3c2e81ef344a
		return -EINVAL;
	}

	if (src_config->flip != EXYNOS_DRM_FLIP_NONE) {
<<<<<<< HEAD
		DRM_DEBUG_KMS("[%s]not support source-side flip\n", __func__);
		return -EINVAL;
	}

	switch (dst_config->flip) {
	case EXYNOS_DRM_FLIP_NONE:
	case EXYNOS_DRM_FLIP_VERTICAL:
	case EXYNOS_DRM_FLIP_HORIZONTAL:
		/* No problem */
		break;
	default:
		DRM_DEBUG_KMS("[%s]invalid flip\n", __func__);
=======
		DRM_DEBUG_KMS("%s:not support source-side flip\n", __func__);
		return -EINVAL;
	}

	if (!rotator_check_drm_flip(dst_config->flip)) {
		DRM_DEBUG_KMS("%s:invalid flip\n", __func__);
>>>>>>> 3c2e81ef344a
		return -EINVAL;
	}

	/* Check size configuration */
	if ((src_pos->x + src_pos->w > src_sz->hsize) ||
		(src_pos->y + src_pos->h > src_sz->vsize)) {
<<<<<<< HEAD
		DRM_DEBUG_KMS("[%s]out of source buffer bound\n", __func__);
=======
		DRM_DEBUG_KMS("%s:out of source buffer bound\n", __func__);
>>>>>>> 3c2e81ef344a
		return -EINVAL;
	}

	if (swap) {
		if ((dst_pos->x + dst_pos->h > dst_sz->vsize) ||
			(dst_pos->y + dst_pos->w > dst_sz->hsize)) {
<<<<<<< HEAD
			DRM_DEBUG_KMS("[%s]out of destination buffer bound\n",
								__func__);
=======
			DRM_DEBUG_KMS("%s:out of destination buffer bound\n",
				__func__);
>>>>>>> 3c2e81ef344a
			return -EINVAL;
		}

		if ((src_pos->w != dst_pos->h) || (src_pos->h != dst_pos->w)) {
<<<<<<< HEAD
			DRM_DEBUG_KMS("[%s]not support scale feature\n",
								__func__);
=======
			DRM_DEBUG_KMS("%s:not support scale feature\n",
				__func__);
>>>>>>> 3c2e81ef344a
			return -EINVAL;
		}
	} else {
		if ((dst_pos->x + dst_pos->w > dst_sz->hsize) ||
			(dst_pos->y + dst_pos->h > dst_sz->vsize)) {
<<<<<<< HEAD
			DRM_DEBUG_KMS("[%s]out of destination buffer bound\n",
								__func__);
=======
			DRM_DEBUG_KMS("%s:out of destination buffer bound\n",
				__func__);
>>>>>>> 3c2e81ef344a
			return -EINVAL;
		}

		if ((src_pos->w != dst_pos->w) || (src_pos->h != dst_pos->h)) {
<<<<<<< HEAD
			DRM_DEBUG_KMS("[%s]not support scale feature\n",
								__func__);
=======
			DRM_DEBUG_KMS("%s:not support scale feature\n",
				__func__);
>>>>>>> 3c2e81ef344a
			return -EINVAL;
		}
	}

	return 0;
}

static int rotator_ippdrv_start(struct device *dev, enum drm_exynos_ipp_cmd cmd)
{
	struct rot_context *rot = dev_get_drvdata(dev);
<<<<<<< HEAD
=======
	u32 val;
>>>>>>> 3c2e81ef344a

	if (rot->suspended) {
		DRM_ERROR("suspended state\n");
		return -EPERM;
	}

	if (cmd != IPP_CMD_M2M) {
		DRM_ERROR("not support cmd: %d\n", cmd);
		return -EINVAL;
	}

	/* Set interrupt enable */
	rotator_reg_set_irq(rot, true);

<<<<<<< HEAD
	/* start rotator operation */
	rotator_reg_set_start(rot);
=======
	val = rot_read(ROT_CONTROL);
	val |= ROT_CONTROL_START;

	rot_write(val, ROT_CONTROL);
>>>>>>> 3c2e81ef344a

	return 0;
}

static int __devinit rotator_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct rot_context *rot;
<<<<<<< HEAD
	struct resource *res;
	struct exynos_drm_ippdrv *ippdrv;
	int ret;

	rot = kzalloc(sizeof(*rot), GFP_KERNEL);
=======
	struct exynos_drm_ippdrv *ippdrv;
	int ret;

	rot = devm_kzalloc(dev, sizeof(*rot), GFP_KERNEL);
>>>>>>> 3c2e81ef344a
	if (!rot) {
		dev_err(dev, "failed to allocate rot\n");
		return -ENOMEM;
	}

	rot->limit_tbl = (struct rot_limit_table *)
				platform_get_device_id(pdev)->driver_data;

<<<<<<< HEAD
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "failed to find registers\n");
		ret = -ENOENT;
		goto err_get_resource;
	}

	rot->regs_res = request_mem_region(res->start, resource_size(res),
								dev_name(dev));
	if (!rot->regs_res) {
		dev_err(dev, "failed to claim register region\n");
=======
	rot->regs_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!rot->regs_res) {
		dev_err(dev, "failed to find registers\n");
>>>>>>> 3c2e81ef344a
		ret = -ENOENT;
		goto err_get_resource;
	}

<<<<<<< HEAD
	rot->regs = ioremap(res->start, resource_size(res));
	if (!rot->regs) {
		dev_err(dev, "failed to map register\n");
		ret = -ENXIO;
		goto err_ioremap;
=======
	rot->regs = devm_request_and_ioremap(dev, rot->regs_res);
	if (!rot->regs) {
		dev_err(dev, "failed to map register\n");
		ret = -ENXIO;
		goto err_get_resource;
>>>>>>> 3c2e81ef344a
	}

	rot->irq = platform_get_irq(pdev, 0);
	if (rot->irq < 0) {
<<<<<<< HEAD
		dev_err(dev, "faild to get irq\n");
=======
		dev_err(dev, "failed to get irq\n");
>>>>>>> 3c2e81ef344a
		ret = rot->irq;
		goto err_get_irq;
	}

	ret = request_threaded_irq(rot->irq, NULL, rotator_irq_handler,
<<<<<<< HEAD
					IRQF_ONESHOT, "drm_rotator", rot);
=======
			IRQF_ONESHOT, "drm_rotator", rot);
>>>>>>> 3c2e81ef344a
	if (ret < 0) {
		dev_err(dev, "failed to request irq\n");
		goto err_get_irq;
	}

	rot->clock = clk_get(dev, "rotator");
	if (IS_ERR_OR_NULL(rot->clock)) {
<<<<<<< HEAD
		dev_err(dev, "faild to get clock\n");
=======
		dev_err(dev, "failed to get clock\n");
>>>>>>> 3c2e81ef344a
		ret = PTR_ERR(rot->clock);
		goto err_clk_get;
	}

	pm_runtime_enable(dev);

	ippdrv = &rot->ippdrv;
	ippdrv->dev = dev;
<<<<<<< HEAD
	ippdrv->iommu_used = true;
=======
>>>>>>> 3c2e81ef344a
	ippdrv->ops[EXYNOS_DRM_OPS_SRC] = &rot_src_ops;
	ippdrv->ops[EXYNOS_DRM_OPS_DST] = &rot_dst_ops;
	ippdrv->check_property = rotator_ippdrv_check_property;
	ippdrv->start = rotator_ippdrv_start;
<<<<<<< HEAD
=======
	ret = rotator_init_prop_list(ippdrv);
	if (ret < 0) {
		dev_err(dev, "failed to init property list.\n");
		goto err_ippdrv_register;
	}

	DRM_DEBUG_KMS("%s:ippdrv[0x%x]\n", __func__, (int)ippdrv);
>>>>>>> 3c2e81ef344a

	platform_set_drvdata(pdev, rot);

	ret = exynos_drm_ippdrv_register(ippdrv);
	if (ret < 0) {
		dev_err(dev, "failed to register drm rotator device\n");
		goto err_ippdrv_register;
	}

	dev_info(dev, "The exynos rotator is probed successfully\n");

	return 0;

err_ippdrv_register:
<<<<<<< HEAD
=======
	devm_kfree(dev, ippdrv->prop_list);
>>>>>>> 3c2e81ef344a
	pm_runtime_disable(dev);
	clk_put(rot->clock);
err_clk_get:
	free_irq(rot->irq, rot);
err_get_irq:
<<<<<<< HEAD
	iounmap(rot->regs);
err_ioremap:
	release_resource(rot->regs_res);
	kfree(rot->regs_res);
err_get_resource:
	kfree(rot);
=======
	devm_iounmap(dev, rot->regs);
err_get_resource:
	devm_kfree(dev, rot);
>>>>>>> 3c2e81ef344a
	return ret;
}

static int __devexit rotator_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct rot_context *rot = dev_get_drvdata(dev);
<<<<<<< HEAD

	exynos_drm_ippdrv_unregister(&rot->ippdrv);
=======
	struct exynos_drm_ippdrv *ippdrv = &rot->ippdrv;

	devm_kfree(dev, ippdrv->prop_list);
	exynos_drm_ippdrv_unregister(ippdrv);
>>>>>>> 3c2e81ef344a

	pm_runtime_disable(dev);
	clk_put(rot->clock);

	free_irq(rot->irq, rot);
<<<<<<< HEAD

	iounmap(rot->regs);

	release_resource(rot->regs_res);
	kfree(rot->regs_res);

	kfree(rot);
=======
	devm_iounmap(dev, rot->regs);

	devm_kfree(dev, rot);
>>>>>>> 3c2e81ef344a

	return 0;
}

struct rot_limit_table rot_limit_tbl = {
	.ycbcr420_2p = {
		.min_w = 32,
		.min_h = 32,
		.max_w = SZ_32K,
		.max_h = SZ_32K,
		.align = 3,
	},
	.rgb888 = {
		.min_w = 8,
		.min_h = 8,
		.max_w = SZ_8K,
		.max_h = SZ_8K,
		.align = 2,
	},
};

struct platform_device_id rotator_driver_ids[] = {
	{
		.name		= "exynos-rot",
		.driver_data	= (unsigned long)&rot_limit_tbl,
	},
	{},
};

<<<<<<< HEAD
=======
static int rotator_clk_crtl(struct rot_context *rot, bool enable)
{
	DRM_DEBUG_KMS("%s\n", __func__);

	if (enable) {
		clk_enable(rot->clock);
		rot->suspended = false;
	} else {
		clk_disable(rot->clock);
		rot->suspended = true;
	}

	return 0;
}


>>>>>>> 3c2e81ef344a
#ifdef CONFIG_PM_SLEEP
static int rotator_suspend(struct device *dev)
{
	struct rot_context *rot = dev_get_drvdata(dev);
<<<<<<< HEAD
	struct exynos_drm_ippdrv *ippdrv = &rot->ippdrv;
	struct drm_device *drm_dev = ippdrv->drm_dev;
	struct exynos_drm_private *drm_priv = drm_dev->dev_private;

	rot->suspended = true;

	exynos_drm_iommu_deactivate(drm_priv->vmm, dev);

	return 0;
=======

	DRM_DEBUG_KMS("%s\n", __func__);

	if (pm_runtime_suspended(dev))
		return 0;

	return rotator_clk_crtl(rot, false);
>>>>>>> 3c2e81ef344a
}

static int rotator_resume(struct device *dev)
{
	struct rot_context *rot = dev_get_drvdata(dev);
<<<<<<< HEAD
	struct exynos_drm_ippdrv *ippdrv = &rot->ippdrv;
	struct drm_device *drm_dev = ippdrv->drm_dev;
	struct exynos_drm_private *drm_priv = drm_dev->dev_private;
	int ret;

	ret = exynos_drm_iommu_activate(drm_priv->vmm, dev);
	if (ret)
		DRM_ERROR("failed to activate iommu\n");

	rot->suspended = false;

	return ret;
=======

	DRM_DEBUG_KMS("%s\n", __func__);

	if (!pm_runtime_suspended(dev))
		return rotator_clk_crtl(rot, true);

	return 0;
>>>>>>> 3c2e81ef344a
}
#endif

#ifdef CONFIG_PM_RUNTIME
static int rotator_runtime_suspend(struct device *dev)
{
	struct rot_context *rot = dev_get_drvdata(dev);

<<<<<<< HEAD
	clk_disable(rot->clock);

	return 0;
=======
	DRM_DEBUG_KMS("%s\n", __func__);

	return  rotator_clk_crtl(rot, false);
>>>>>>> 3c2e81ef344a
}

static int rotator_runtime_resume(struct device *dev)
{
	struct rot_context *rot = dev_get_drvdata(dev);

<<<<<<< HEAD
	clk_enable(rot->clock);

	return 0;
=======
	DRM_DEBUG_KMS("%s\n", __func__);

	return  rotator_clk_crtl(rot, true);
>>>>>>> 3c2e81ef344a
}
#endif

static const struct dev_pm_ops rotator_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(rotator_suspend, rotator_resume)
	SET_RUNTIME_PM_OPS(rotator_runtime_suspend, rotator_runtime_resume,
									NULL)
};

struct platform_driver rotator_driver = {
	.probe		= rotator_probe,
	.remove		= __devexit_p(rotator_remove),
	.id_table	= rotator_driver_ids,
	.driver		= {
		.name	= "exynos-rot",
		.owner	= THIS_MODULE,
		.pm	= &rotator_pm_ops,
	},
};
