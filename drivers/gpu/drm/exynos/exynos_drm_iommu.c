/* exynos_drm_iommu.c
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 * Author: Inki Dae <inki.dae@samsung.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

<<<<<<< HEAD
#include "drmP.h"
#include "drm.h"

#include <drm/exynos_drm.h>

#include <plat/s5p-iovmm.h>

#include "exynos_drm_drv.h"
#include "exynos_drm_gem.h"
#include "exynos_drm_iommu.h"

static DEFINE_MUTEX(iommu_mutex);

struct exynos_iommu_ops {
	void *(*setup)(unsigned long s_iova, unsigned long size);
	void (*cleanup)(void *in_vmm);
	int (*activate)(void *in_vmm, struct device *dev);
	void (*deactivate)(void *in_vmm, struct device *dev);
	dma_addr_t (*map)(void *in_vmm, struct scatterlist *sg,
				off_t offset, size_t size);
	void (*unmap)(void *in_vmm, dma_addr_t iova);
};

static const struct exynos_iommu_ops iommu_ops = {
	.setup		= iovmm_setup,
	.cleanup	= iovmm_cleanup,
	.activate	= iovmm_activate,
	.deactivate	= iovmm_deactivate,
	.map		= iovmm_map,
	.unmap		= iovmm_unmap
};

dma_addr_t exynos_drm_iommu_map_gem(struct drm_device *drm_dev,
					struct drm_gem_object *obj)
{
	struct exynos_drm_gem_obj *exynos_gem_obj;
	struct exynos_drm_gem_buf *buf;
	struct sg_table *sgt;
	dma_addr_t dev_addr;

	mutex_lock(&iommu_mutex);

	exynos_gem_obj = to_exynos_gem_obj(obj);

	buf = exynos_gem_obj->buffer;
	sgt = buf->sgt;

	/*
	 * if not using iommu, just return base address to physical
	 * memory region of the gem.
	 */
	if (!iommu_ops.map) {
		mutex_unlock(&iommu_mutex);
		return sg_dma_address(&sgt->sgl[0]);
	}

	/*
	 * if a gem buffer was already mapped with iommu table then
	 * just return dev_addr;
	 *
	 * Note: device address is unique to system globally.
	 */
	if (buf->dev_addr) {
		mutex_unlock(&iommu_mutex);
		return buf->dev_addr;
	}

	/*
	 * allocate device address space for this driver and then
	 * map all pages contained in sg list to iommu table.
	 */
	dev_addr = iommu_ops.map(exynos_gem_obj->vmm, sgt->sgl, (off_t)0,
					(size_t)obj->size);
	if (!dev_addr) {
		mutex_unlock(&iommu_mutex);
		return dev_addr;
	}

	mutex_unlock(&iommu_mutex);

	return dev_addr;
}

void exynos_drm_iommu_unmap_gem(struct drm_gem_object *obj)
{
	struct exynos_drm_gem_obj *exynos_gem_obj;
	struct exynos_drm_gem_buf *buf;

	if (!iommu_ops.unmap || !obj)
		return;

	exynos_gem_obj = to_exynos_gem_obj(obj);
	buf = exynos_gem_obj->buffer;

	/* workaround */
	usleep_range(15000, 20000);

	mutex_lock(&iommu_mutex);

	if (!buf->dev_addr) {
		mutex_unlock(&iommu_mutex);
		DRM_DEBUG_KMS("not mapped with iommu table.\n");
		return;
	}

	if (exynos_gem_obj->vmm)
		iommu_ops.unmap(exynos_gem_obj->vmm, buf->dev_addr);

	buf->dev_addr = 0;
	mutex_unlock(&iommu_mutex);
}

dma_addr_t exynos_drm_iommu_map(void *in_vmm, dma_addr_t paddr,
				size_t size)
{
	struct sg_table *sgt;
	struct scatterlist *sgl;
	dma_addr_t dma_addr = 0, tmp_addr;
	unsigned int npages, i = 0;
	int ret;

	 /* if not using iommu, just return paddr. */
	if (!iommu_ops.map)
		return paddr;

	npages = size >> PAGE_SHIFT;

	sgt = kzalloc(sizeof(struct sg_table) * npages, GFP_KERNEL);
	if (!sgt) {
		DRM_ERROR("failed to allocate sg table.\n");
		return dma_addr;
	}

	ret = sg_alloc_table(sgt, npages, GFP_KERNEL);
	if (ret < 0) {
		DRM_ERROR("failed to initialize sg table.\n");
		goto err;
	}

	sgl = sgt->sgl;
	tmp_addr = paddr;

	while (i < npages) {
		struct page *page = phys_to_page(tmp_addr);
		sg_set_page(sgl, page, PAGE_SIZE, 0);
		sg_dma_len(sgl) = PAGE_SIZE;
		tmp_addr += PAGE_SIZE;
		i++;
		sgl = sg_next(sgl);
	}

	/*
	 * allocate device address space for this driver and then
	 * map all pages contained in sg list to iommu table.
	 */
	dma_addr = iommu_ops.map(in_vmm, sgt->sgl, (off_t)0, (size_t)size);
	if (!dma_addr)
		DRM_ERROR("failed to map cmdlist pool.\n");

	sg_free_table(sgt);
err:
	kfree(sgt);
	sgt = NULL;

	return dma_addr;
}


void exynos_drm_iommu_unmap(void *in_vmm, dma_addr_t dma_addr)
{
	if (iommu_ops.unmap)
		iommu_ops.unmap(in_vmm, dma_addr);
}

void *exynos_drm_iommu_setup(unsigned long s_iova, unsigned long size)
{
	/*
	 * allocate device address space to this driver and add vmm object
	 * to s5p_iovmm_list. please know that each iommu will use
	 * 1GB as its own device address apace.
	 *
	 * the device address space : s_iova ~ s_iova + size
	 */
	if (iommu_ops.setup)
		return iommu_ops.setup(s_iova, size);

	return ERR_PTR(-EINVAL);
}

int exynos_drm_iommu_activate(void *in_vmm, struct device *dev)
{
	if (iommu_ops.activate)
		return iovmm_activate(in_vmm, dev);
=======
#include <drmP.h>
#include <drm/exynos_drm.h>

#include <linux/dma-mapping.h>
#include <linux/iommu.h>
#include <linux/kref.h>

#include <asm/dma-iommu.h>

#include "exynos_drm_drv.h"
#include "exynos_drm_iommu.h"

/*
 * drm_create_iommu_mapping - create a mapping structure
 *
 * @drm_dev: DRM device
 */
int drm_create_iommu_mapping(struct drm_device *drm_dev)
{
	struct dma_iommu_mapping *mapping = NULL;
	struct exynos_drm_private *priv = drm_dev->dev_private;
	struct device *dev = drm_dev->dev;

	if (!priv->da_start)
		priv->da_start = EXYNOS_DEV_ADDR_START;
	if (!priv->da_space_size)
		priv->da_space_size = EXYNOS_DEV_ADDR_SIZE;
	if (!priv->da_space_order)
		priv->da_space_order = EXYNOS_DEV_ADDR_ORDER;

	mapping = arm_iommu_create_mapping(&platform_bus_type, priv->da_start,
						priv->da_space_size,
						priv->da_space_order);
	if (IS_ERR(mapping))
		return PTR_ERR(mapping);

	dev->dma_parms = devm_kzalloc(dev, sizeof(*dev->dma_parms),
					GFP_KERNEL);
	dma_set_max_seg_size(dev, 0xffffffffu);
	dev->archdata.mapping = mapping;

	return 0;
}

/*
 * drm_release_iommu_mapping - release iommu mapping structure
 *
 * @drm_dev: DRM device
 *
 * if mapping->kref becomes 0 then all things related to iommu mapping
 * will be released
 */
void drm_release_iommu_mapping(struct drm_device *drm_dev)
{
	struct device *dev = drm_dev->dev;

	arm_iommu_release_mapping(dev->archdata.mapping);
}

/*
 * drm_iommu_attach_device- attach device to iommu mapping
 *
 * @drm_dev: DRM device
 * @subdrv_dev: device to be attach
 *
 * This function should be called by sub drivers to attach it to iommu
 * mapping.
 */
int drm_iommu_attach_device(struct drm_device *drm_dev,
				struct device *subdrv_dev)
{
	struct device *dev = drm_dev->dev;
	int ret;

	if (!dev->archdata.mapping) {
		DRM_ERROR("iommu_mapping is null.\n");
		return -EFAULT;
	}

	subdrv_dev->dma_parms = devm_kzalloc(subdrv_dev,
					sizeof(*subdrv_dev->dma_parms),
					GFP_KERNEL);
	dma_set_max_seg_size(subdrv_dev, 0xffffffffu);

	ret = arm_iommu_attach_device(subdrv_dev, dev->archdata.mapping);
	if (ret < 0) {
		DRM_DEBUG_KMS("failed iommu attach.\n");
		return ret;
	}

	/*
	 * Set dma_ops to drm_device just one time.
	 *
	 * The dma mapping api needs device object and the api is used
	 * to allocate physial memory and map it with iommu table.
	 * If iommu attach succeeded, the sub driver would have dma_ops
	 * for iommu and also all sub drivers have same dma_ops.
	 */
	if (!dev->archdata.dma_ops)
		dev->archdata.dma_ops = subdrv_dev->archdata.dma_ops;
>>>>>>> 3c2e81ef344a

	return 0;
}

<<<<<<< HEAD
void exynos_drm_iommu_deactivate(void *in_vmm, struct device *dev)
{
	if (iommu_ops.deactivate)
		iommu_ops.deactivate(in_vmm, dev);
}

void exynos_drm_iommu_cleanup(void *in_vmm)
{
	if (iommu_ops.cleanup)
		iommu_ops.cleanup(in_vmm);
}

MODULE_AUTHOR("Inki Dae <inki.dae@samsung.com>");
MODULE_DESCRIPTION("Samsung SoC DRM IOMMU Framework");
MODULE_LICENSE("GPL");
=======
/*
 * drm_iommu_detach_device -detach device address space mapping from device
 *
 * @drm_dev: DRM device
 * @subdrv_dev: device to be detached
 *
 * This function should be called by sub drivers to detach it from iommu
 * mapping
 */
void drm_iommu_detach_device(struct drm_device *drm_dev,
				struct device *subdrv_dev)
{
	struct device *dev = drm_dev->dev;
	struct dma_iommu_mapping *mapping = dev->archdata.mapping;

	if (!mapping || !mapping->domain)
		return;

	iommu_detach_device(mapping->domain, subdrv_dev);
	drm_release_iommu_mapping(drm_dev);
}
>>>>>>> 3c2e81ef344a
