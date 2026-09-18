/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Shared Memory Driver IOCTL Definitions
 */

#ifndef _SHARED_MEMORY_IOCTL_H_
#define _SHARED_MEMORY_IOCTL_H_

#include <linux/ioctl.h>
#include <linux/types.h>

#define SHMEM_IOC_MAGIC  'S'

/* IOCTL command to get the number of memory regions */
#define SHMEM_IOC_GET_REGION_COUNT   _IOR(SHMEM_IOC_MAGIC, 1, int)

/* Structure for memory region information */
struct shmem_region_info {
	__u32 index;        /* Region index (input) */
	__u64 phys_addr;    /* Physical address (output) */
	__u64 size;         /* Region size (output) */
	__u32 align;        /* Alignment requirement (output) */
	__u32 reserved;     /* Reserved for future use */
};

/* IOCTL command to get region information */
#define SHMEM_IOC_GET_REGION_INFO    _IOWR(SHMEM_IOC_MAGIC, 2, struct shmem_region_info)

#endif /* _SHARED_MEMORY_IOCTL_H_ */
