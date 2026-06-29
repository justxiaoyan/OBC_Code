#include <errno.h>
#include <fcntl.h>
#include <linux/mmc/ioctl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "generic/generic.h"
#include "longsys/longsys.h"
#include "micron/micron.h"
#include "samsung/samsung.h"
#include "smart_vendor.h"

#define MMC_SEND_EXT_CSD 8
#define MMC_RSP_PRESENT (1 << 0)
#define MMC_RSP_CRC (1 << 2)
#define MMC_RSP_OPCODE (1 << 4)
#define MMC_CMD_ADTC (1 << 5)
#define MMC_RSP_R1 (MMC_RSP_PRESENT | MMC_RSP_CRC | MMC_RSP_OPCODE)
#define MMC_EXT_CSD_FLAGS (MMC_RSP_R1 | MMC_CMD_ADTC)

#define SYSFS_VALUE_MAX 128
#define DEVICE_PATH_MAX 256

static const struct smart_vendor *const vendor_table[] = {
	&longsys_vendor,
	&micron_vendor,
	&samsung_vendor,
	&generic_vendor,  /* Always last as fallback */
};

static void usage(const char *program)
{
	fprintf(stderr,
		"Usage: %s [--vendor longsys|micron|samsung|generic|auto] <mmcblkN|/dev/mmcblkN>\n"
		"\n"
		"Vendor profiles:\n"
		"  longsys  - Jiangbolong/Longsys/Foresee eMMC\n"
		"  micron   - Micron eMMC\n"
		"  samsung  - Samsung eMMC\n"
		"  generic  - Generic JEDEC eMMC (fallback)\n"
		"  auto     - Auto-detect vendor (or manual selection)\n"
		"\n"
		"Examples:\n"
		"  %s mmcblk0\n"
		"  %s --vendor longsys /dev/mmcblk0\n"
		"  %s --vendor samsung mmcblk0\n",
		program, program, program, program);
}

static int build_device_path(const char *arg, char *path, size_t path_len)
{
	int ret;

	if (strncmp(arg, "/dev/", 5) == 0)
		ret = snprintf(path, path_len, "%s", arg);
	else
		ret = snprintf(path, path_len, "/dev/%s", arg);

	if (ret < 0 || (size_t)ret >= path_len) {
		fprintf(stderr, "Device path is too long: %s\n", arg);
		return -1;
	}

	return 0;
}

static const char *block_name_from_path(const char *path)
{
	const char *slash = strrchr(path, '/');

	return slash ? slash + 1 : path;
}

static int read_sysfs_value(const char *block_name, const char *file,
			    char *buf, size_t buf_len)
{
	char path[DEVICE_PATH_MAX];
	int ret;
	FILE *fp;
	size_t len;

	ret = snprintf(path, sizeof(path), "/sys/block/%s/device/%s",
		       block_name, file);
	if (ret < 0 || (size_t)ret >= sizeof(path))
		return -1;

	fp = fopen(path, "r");
	if (!fp)
		return -1;

	if (!fgets(buf, buf_len, fp)) {
		fclose(fp);
		return -1;
	}

	fclose(fp);
	len = strlen(buf);
	while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r' ||
			   buf[len - 1] == ' ' || buf[len - 1] == '\t')) {
		buf[len - 1] = '\0';
		len--;
	}

	return 0;
}

static int read_ext_csd(int fd, uint8_t ext_csd[EMMC_EXT_CSD_SIZE])
{
	struct mmc_ioc_cmd cmd;

	memset(&cmd, 0, sizeof(cmd));
	memset(ext_csd, 0, EMMC_EXT_CSD_SIZE);

	cmd.write_flag = 0;
	cmd.opcode = MMC_SEND_EXT_CSD;
	cmd.arg = 0;
	cmd.flags = MMC_EXT_CSD_FLAGS;
	cmd.blksz = EMMC_EXT_CSD_SIZE;
	cmd.blocks = 1;
	mmc_ioc_cmd_set_data(cmd, ext_csd);

	if (ioctl(fd, MMC_IOC_CMD, &cmd) < 0)
		return -1;

	return 0;
}

const struct smart_vendor *smart_vendor_find(const char *requested,
					     const struct smart_device_info *info)
{
	size_t i;

	if (requested && strcmp(requested, "auto") != 0) {
		for (i = 0; i < sizeof(vendor_table) / sizeof(vendor_table[0]); i++) {
			if (strcmp(requested, vendor_table[i]->name) == 0)
				return vendor_table[i];
		}

		return NULL;
	}

	for (i = 0; i < sizeof(vendor_table) / sizeof(vendor_table[0]); i++) {
		if (vendor_table[i]->match && vendor_table[i]->match(info))
			return vendor_table[i];
	}

	if (sizeof(vendor_table) / sizeof(vendor_table[0]) == 1)
		return vendor_table[0];

	return NULL;
}

static void print_device_header(const struct smart_device_info *info,
				const char *device_path)
{
	printf("Device        : %s\n", device_path);
	printf("Block name    : %s\n", info->block_name);
	if (info->sysfs_name && info->sysfs_name[0] != '\0')
		printf("Product name  : %s\n", info->sysfs_name);
	if (info->manfid && info->manfid[0] != '\0')
		printf("Manufacturer  : %s\n", info->manfid);
	if (info->cid && info->cid[0] != '\0')
		printf("CID           : %s\n", info->cid);
	printf("\n");
}

static const struct smart_vendor *prompt_vendor_selection(void)
{
	size_t i;
	int choice;
	char input[16];

	printf("Auto-detection failed. Please select vendor manually:\n\n");

	for (i = 0; i < sizeof(vendor_table) / sizeof(vendor_table[0]); i++) {
		printf("  [%zu] %s\n", i + 1, vendor_table[i]->display_name);
	}

	printf("\nEnter selection (1-%zu): ", sizeof(vendor_table) / sizeof(vendor_table[0]));
	fflush(stdout);

	if (!fgets(input, sizeof(input), stdin)) {
		fprintf(stderr, "Failed to read input\n");
		return NULL;
	}

	choice = atoi(input);
	if (choice < 1 || (size_t)choice > sizeof(vendor_table) / sizeof(vendor_table[0])) {
		fprintf(stderr, "Invalid selection: %d\n", choice);
		return NULL;
	}

	return vendor_table[choice - 1];
}

int main(int argc, char **argv)
{
	const char *vendor_name = "auto";
	const char *device_arg = NULL;
	char device_path[DEVICE_PATH_MAX];
	char sysfs_name[SYSFS_VALUE_MAX] = "";
	char manfid[SYSFS_VALUE_MAX] = "";
	char cid[SYSFS_VALUE_MAX] = "";
	uint8_t ext_csd[EMMC_EXT_CSD_SIZE];
	struct smart_device_info info;
	const struct smart_vendor *vendor;
	int fd;
	int i;

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--vendor") == 0) {
			if (i + 1 >= argc) {
				usage(argv[0]);
				return EXIT_FAILURE;
			}
			vendor_name = argv[++i];
		} else if (strcmp(argv[i], "-h") == 0 ||
			   strcmp(argv[i], "--help") == 0) {
			usage(argv[0]);
			return EXIT_SUCCESS;
		} else if (!device_arg) {
			device_arg = argv[i];
		} else {
			usage(argv[0]);
			return EXIT_FAILURE;
		}
	}

	if (!device_arg) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	if (build_device_path(device_arg, device_path, sizeof(device_path)) < 0)
		return EXIT_FAILURE;

	fd = open(device_path, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Failed to open %s: %s\n", device_path,
			strerror(errno));
		return EXIT_FAILURE;
	}

	if (read_ext_csd(fd, ext_csd) < 0) {
		fprintf(stderr, "Failed to read EXT_CSD from %s: %s\n",
			device_path, strerror(errno));
		close(fd);
		return EXIT_FAILURE;
	}

	close(fd);

	info.block_name = block_name_from_path(device_path);
	read_sysfs_value(info.block_name, "name", sysfs_name, sizeof(sysfs_name));
	read_sysfs_value(info.block_name, "manfid", manfid, sizeof(manfid));
	read_sysfs_value(info.block_name, "cid", cid, sizeof(cid));
	info.sysfs_name = sysfs_name;
	info.manfid = manfid;
	info.cid = cid;
	info.ext_csd = ext_csd;
	info.ext_csd_len = sizeof(ext_csd);

	vendor = smart_vendor_find(vendor_name, &info);
	if (!vendor) {
		if (strcmp(vendor_name, "auto") == 0) {
			/* Auto-detection failed, prompt user to select manually */
			vendor = prompt_vendor_selection();
			if (!vendor) {
				fprintf(stderr, "No vendor selected\n");
				return EXIT_FAILURE;
			}
		} else {
			fprintf(stderr, "Unsupported vendor profile: %s\n", vendor_name);
			fprintf(stderr, "Supported profiles: auto, longsys, micron, samsung, generic\n");
			return EXIT_FAILURE;
		}
	}

	print_device_header(&info, device_path);
	vendor->print(&info, stdout);

	return EXIT_SUCCESS;
}
