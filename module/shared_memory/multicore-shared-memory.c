// SPDX-License-Identifier: GPL-2.0
/*
 * Multicore Shared Memory Driver
 * Independent implementation for OBC platform
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_reserved_mem.h>
#include <linux/platform_device.h>
#include <linux/version.h>

#include "shared_memory_ioctl.h"

#define DRIVER_NAME "shared_memory"
#define DEVICE_NAME "shared_memory"

struct shmem_region {
	unsigned int align;
	struct resource res;
	phys_addr_t phys_addr;
	size_t size;
};

struct shmem_device {
	struct device *dev;
	int nr_regions;
	struct shmem_region *regions;
	struct miscdevice miscdev;
};

#define to_shmem_device(n)	container_of(n, struct shmem_device, miscdev)

static struct shmem_device *g_shmem_dev = NULL;

/* Find the region that contains the given offset and length */
static struct shmem_region *shmem_region_find(struct shmem_device *shmem_dev,
					     unsigned long pgoff, unsigned long len)
{
	unsigned long offset = pgoff << PAGE_SHIFT;
	struct shmem_region *region;
	int i;

	for (i = 0; i < shmem_dev->nr_regions; i++) {
		region = &shmem_dev->regions[i];
		if (offset >= region->res.start &&
		    (offset + len) <= (region->res.start + resource_size(&region->res))) {
			return region;
		}
	}

	return NULL;
}

/* mmap implementation */
static int shmem_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct miscdevice *miscdev = filp->private_data;
	struct shmem_device *shmem_dev = to_shmem_device(miscdev);
	struct shmem_region *region;
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
	unsigned long size = vma->vm_end - vma->vm_start;
	unsigned long pfn;

	region = shmem_region_find(shmem_dev, vma->vm_pgoff, size);
	if (!region) {
		dev_err(shmem_dev->dev, "Invalid memory region for offset 0x%lx, size 0x%lx\n",
			offset, size);
		return -EINVAL;
	}

	/* Calculate the physical frame number */
	pfn = (region->res.start + (offset - region->res.start)) >> PAGE_SHIFT;

	/* Set VM flags */
	vm_flags_set(vma, VM_IO | VM_DONTEXPAND | VM_DONTDUMP);
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	/* Map the physical memory */
	if (remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot)) {
		dev_err(shmem_dev->dev, "Failed to remap memory region\n");
		return -EAGAIN;
	}

	dev_dbg(shmem_dev->dev, "mmap: offset=0x%lx, size=0x%lx, pfn=0x%lx\n",
		offset, size, pfn);

	return 0;
}

/* ioctl implementation */
static long shmem_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct miscdevice *miscdev = filp->private_data;
	struct shmem_device *shmem_dev = to_shmem_device(miscdev);
	void __user *argp = (void __user *)arg;
	int ret = 0;

	switch (cmd) {
	case SHMEM_IOC_GET_REGION_COUNT: {
		if (copy_to_user(argp, &shmem_dev->nr_regions, sizeof(int)))
			ret = -EFAULT;
		break;
	}
	case SHMEM_IOC_GET_REGION_INFO: {
		struct shmem_region_info info;
		struct shmem_region *region;

		if (copy_from_user(&info, argp, sizeof(info))) {
			ret = -EFAULT;
			break;
		}

		if (info.index >= shmem_dev->nr_regions) {
			ret = -EINVAL;
			break;
		}

		region = &shmem_dev->regions[info.index];
		info.phys_addr = region->res.start;
		info.size = resource_size(&region->res);
		info.align = region->align;

		if (copy_to_user(argp, &info, sizeof(info)))
			ret = -EFAULT;
		break;
	}
	default:
		ret = -ENOTTY;
		break;
	}

	return ret;
}

static const struct file_operations shmem_fops = {
	.owner = THIS_MODULE,
	.mmap = shmem_mmap,
	.unlocked_ioctl = shmem_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = shmem_ioctl,
#endif
};

/* Parse device tree to get memory regions */
static int shmem_parse_dt(struct platform_device *pdev, struct shmem_device *shmem_dev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct device_node *rmem_np;
	struct reserved_mem *rmem;
	int count, i, ret;

	if (!np) {
		dev_err(dev, "No device tree node found\n");
		return -ENODEV;
	}

	/* Count memory-region references */
	count = of_count_phandle_with_args(np, "memory-region", NULL);
	if (count <= 0) {
		dev_err(dev, "No memory-region property found\n");
		return -EINVAL;
	}

	shmem_dev->nr_regions = count;
	shmem_dev->regions = devm_kcalloc(dev, count, sizeof(struct shmem_region), GFP_KERNEL);
	if (!shmem_dev->regions)
		return -ENOMEM;

	/* Parse each memory region */
	for (i = 0; i < count; i++) {
		struct shmem_region *region = &shmem_dev->regions[i];

		rmem_np = of_parse_phandle(np, "memory-region", i);
		if (!rmem_np) {
			dev_err(dev, "Failed to parse memory-region[%d]\n", i);
			return -EINVAL;
		}

		/* Get alignment if specified */
		ret = of_property_read_u32(rmem_np, "alignment", &region->align);
		if (ret || region->align == 0)
			region->align = PAGE_SIZE;

		/* Get reserved memory region */
		rmem = of_reserved_mem_lookup(rmem_np);
		if (!rmem) {
			dev_err(dev, "Failed to lookup reserved memory[%d]\n", i);
			of_node_put(rmem_np);
			return -EINVAL;
		}

		region->res.start = rmem->base;
		region->res.end = rmem->base + rmem->size - 1;
		region->res.flags = IORESOURCE_MEM;
		region->phys_addr = rmem->base;
		region->size = rmem->size;

		dev_info(dev, "Region[%d]: phys=0x%llx, size=0x%lx, align=0x%x\n",
			 i, (unsigned long long)region->phys_addr,
			 (unsigned long)region->size, region->align);

		of_node_put(rmem_np);
	}

	return 0;
}

/* Platform device probe */
static int shmem_probe(struct platform_device *pdev)
{
	struct shmem_device *shmem_dev;
	int ret;

	dev_info(&pdev->dev, "Probing shared memory driver\n");

	shmem_dev = devm_kzalloc(&pdev->dev, sizeof(*shmem_dev), GFP_KERNEL);
	if (!shmem_dev)
		return -ENOMEM;

	shmem_dev->dev = &pdev->dev;

	/* Parse device tree */
	ret = shmem_parse_dt(pdev, shmem_dev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to parse device tree: %d\n", ret);
		return ret;
	}

	/* Register misc device */
	shmem_dev->miscdev.minor = MISC_DYNAMIC_MINOR;
	shmem_dev->miscdev.name = DEVICE_NAME;
	shmem_dev->miscdev.fops = &shmem_fops;
	shmem_dev->miscdev.parent = &pdev->dev;

	ret = misc_register(&shmem_dev->miscdev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register misc device: %d\n", ret);
		return ret;
	}

	platform_set_drvdata(pdev, shmem_dev);
	g_shmem_dev = shmem_dev;

	dev_info(&pdev->dev, "Shared memory driver registered successfully\n");
	dev_info(&pdev->dev, "Device node: /dev/%s\n", DEVICE_NAME);

	return 0;
}

/* Platform device remove */
static void shmem_remove(struct platform_device *pdev)
{
	struct shmem_device *shmem_dev = platform_get_drvdata(pdev);

	if (shmem_dev) {
		misc_deregister(&shmem_dev->miscdev);
		g_shmem_dev = NULL;
	}

	dev_info(&pdev->dev, "Shared memory driver removed\n");
}

static const struct of_device_id shmem_of_match[] = {
	{ .compatible = "shared-memory", },
	{ .compatible = "obc,shared-memory", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, shmem_of_match);

static struct platform_driver shmem_driver = {
	.probe = shmem_probe,
	.remove = shmem_remove,
	.driver = {
		.name = DRIVER_NAME,
		.of_match_table = shmem_of_match,
	},
};

module_platform_driver(shmem_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("OBC Development Team");
MODULE_DESCRIPTION("Multicore Shared Memory Driver");
MODULE_VERSION("1.0");
