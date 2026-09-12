#ifndef SMART_VENDOR_H
#define SMART_VENDOR_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define EMMC_EXT_CSD_SIZE 512U

struct smart_device_info {
	const char *block_name;
	const char *sysfs_name;
	const char *manfid;
	const char *cid;
	const uint8_t *ext_csd;
	size_t ext_csd_len;
};

struct smart_vendor {
	const char *name;
	const char *display_name;
	int (*match)(const struct smart_device_info *info);
	void (*print)(const struct smart_device_info *info, FILE *out);
};

const struct smart_vendor *smart_vendor_find(const char *requested,
					     const struct smart_device_info *info);

#endif
