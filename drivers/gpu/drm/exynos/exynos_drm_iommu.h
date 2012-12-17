/* exynos_drm_iommu.h
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 * Authoer: Inki Dae <inki.dae@samsung.com>
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

#ifndef _EXYNOS_DRM_IOMMU_H_
#define _EXYNOS_DRM_IOMMU_H_

<<<<<<< HEAD
struct exynos_iommu_gem_data {
	unsigned int	gem_handle_in;
	void		*gem_obj_out;
};

/* get all pages to gem object and map them to iommu table. */
dma_addr_t exynos_drm_iommu_map_gem(struct drm_device *drm_dev,
					struct drm_gem_object *obj);

/* unmap device address space to gem object from iommu table. */
void exynos_drm_iommu_unmap_gem(struct drm_gem_object *obj);

/* map physical memory region pointed by paddr to iommu table. */
dma_addr_t exynos_drm_iommu_map(void *in_vmm, dma_addr_t paddr,
					size_t size);

/* unmap device address space pointed by dev_addr from iommu table. */
void exynos_drm_iommu_unmap(void *in_vmm, dma_addr_t dev_addr);

/* setup device address space for device iommu. */
void *exynos_drm_iommu_setup(unsigned long s_iova, unsigned long size);

int exynos_drm_iommu_activate(void *in_vmm, struct device *dev);

void exynos_drm_iommu_deactivate(void *in_vmm, struct device *dev);

/* clean up allocated device address space for device iommu. */
void exynos_drm_iommu_cleanup(void *in_vmm);

=======
#define EXYNOS_DEV_ADDR_START	0x20000000
#define EXYNOS_DEV_ADDR_SIZE	0x40000000
#define EXYNOS_DEV_ADDR_ORDER	0x4

#ifdef CONFIG_DRM_EXYNOS_IOMMU

int drm_create_iommu_mapping(struct drm_device *drm_dev);

void drm_release_iommu_mapping(struct drm_device *drm_dev);

int drm_iommu_attach_device(struct drm_device *drm_dev,
				struct device *subdrv_dev);

void drm_iommu_detach_device(struct drm_device *dev_dev,
				struct device *subdrv_dev);

static inline bool is_drm_iommu_supported(struct drm_device *drm_dev)
{
#ifdef CONFIG_ARM_DMA_USE_IOMMU
	struct device *dev = drm_dev->dev;

	return dev->archdata.mapping ? true : false;
#else
	return false;
#endif
}

#else

struct dma_iommu_mapping;
static inline int drm_create_iommu_mapping(struct drm_device *drm_dev)
{
	return 0;
}

static inline void drm_release_iommu_mapping(struct drm_device *drm_dev)
{
}

static inline int drm_iommu_attach_device(struct drm_device *drm_dev,
						struct device *subdrv_dev)
{
	return 0;
}

static inline void drm_iommu_detach_device(struct drm_device *drm_dev,
						struct device *subdrv_dev)
{
}

static inline bool is_drm_iommu_supported(struct drm_device *drm_dev)
{
	return false;
}

#endif
>>>>>>> 3c2e81ef344a
#endif
