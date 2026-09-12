#include "generic.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Standard JEDEC EXT_CSD fields */
#define EXT_CSD_REV 192
#define EXT_CSD_DEVICE_TYPE 196
#define EXT_CSD_PRE_EOL_INFO 267
#define EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_A 268
#define EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_B 269
#define EXT_CSD_SEC_CNT 212
#define EXT_CSD_BKOPS_STATUS 246
#define EXT_CSD_BKOPS_EN 163
#define EXT_CSD_BKOPS_SUPPORT 502
#define EXT_CSD_CACHE_SIZE 249
#define EXT_CSD_CACHE_CTRL 33
#define EXT_CSD_FIRMWARE_VERSION 254
#define EXT_CSD_BOOT_MULT 226
#define EXT_CSD_RPMB_MULT 168
#define EXT_CSD_HC_ERASE_GRP_SIZE 224
#define EXT_CSD_ERASE_TIMEOUT_MULT 223
#define EXT_CSD_TRIM_MULT 232
#define EXT_CSD_CMDQ_SUPPORT 308
#define EXT_CSD_CMDQ_DEPTH 307

struct ext_csd_field {
	const char *name;
	unsigned int offset;
	unsigned int size;
	const char *description;
};

static const struct ext_csd_field generic_fields[] = {
	/* === Health Status === */
	{
		.name = "PRE_EOL_INFO",
		.offset = EXT_CSD_PRE_EOL_INFO,
		.size = 1,
		.description = "Pre-EOL (End of Life) information",
	},
	{
		.name = "DEVICE_LIFE_TIME_EST_TYP_A",
		.offset = EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_A,
		.size = 1,
		.description = "SLC/Enhanced area life time estimation",
	},
	{
		.name = "DEVICE_LIFE_TIME_EST_TYP_B",
		.offset = EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_B,
		.size = 1,
		.description = "MLC/TLC user area life time estimation",
	},

	/* === Capacity Information === */
	{
		.name = "SEC_CNT",
		.offset = EXT_CSD_SEC_CNT,
		.size = 4,
		.description = "Total sector count (x512B = capacity)",
	},

	/* === Background Operations === */
	{
		.name = "BKOPS_STATUS",
		.offset = EXT_CSD_BKOPS_STATUS,
		.size = 1,
		.description = "Background operations status",
	},
	{
		.name = "BKOPS_EN",
		.offset = EXT_CSD_BKOPS_EN,
		.size = 1,
		.description = "Background operations enable",
	},
	{
		.name = "BKOPS_SUPPORT",
		.offset = EXT_CSD_BKOPS_SUPPORT,
		.size = 1,
		.description = "Background operations support",
	},

	/* === Cache Information === */
	{
		.name = "CACHE_SIZE",
		.offset = EXT_CSD_CACHE_SIZE,
		.size = 4,
		.description = "Cache size (x512B)",
	},
	{
		.name = "CACHE_CTRL",
		.offset = EXT_CSD_CACHE_CTRL,
		.size = 1,
		.description = "Cache control (0=OFF, 1=ON)",
	},

	/* === Erase Performance === */
	{
		.name = "HC_ERASE_GRP_SIZE",
		.offset = EXT_CSD_HC_ERASE_GRP_SIZE,
		.size = 1,
		.description = "High-capacity erase group size (x512KB)",
	},
	{
		.name = "ERASE_TIMEOUT_MULT",
		.offset = EXT_CSD_ERASE_TIMEOUT_MULT,
		.size = 1,
		.description = "Erase timeout multiplier",
	},
	{
		.name = "TRIM_MULT",
		.offset = EXT_CSD_TRIM_MULT,
		.size = 1,
		.description = "TRIM operation multiplier",
	},

	/* === Partition Information === */
	{
		.name = "BOOT_MULT",
		.offset = EXT_CSD_BOOT_MULT,
		.size = 1,
		.description = "Boot partition size multiplier (x128KB)",
	},
	{
		.name = "RPMB_MULT",
		.offset = EXT_CSD_RPMB_MULT,
		.size = 1,
		.description = "RPMB partition size multiplier (x128KB)",
	},

	/* === Performance Features === */
	{
		.name = "CMDQ_SUPPORT",
		.offset = EXT_CSD_CMDQ_SUPPORT,
		.size = 1,
		.description = "Command queue support",
	},
	{
		.name = "CMDQ_DEPTH",
		.offset = EXT_CSD_CMDQ_DEPTH,
		.size = 1,
		.description = "Command queue depth",
	},

	/* === Firmware & Version === */
	{
		.name = "FIRMWARE_VERSION",
		.offset = EXT_CSD_FIRMWARE_VERSION,
		.size = 8,
		.description = "Firmware version",
	},
	{
		.name = "EXT_CSD_REV",
		.offset = EXT_CSD_REV,
		.size = 1,
		.description = "Extended CSD revision",
	},
	{
		.name = "DEVICE_TYPE",
		.offset = EXT_CSD_DEVICE_TYPE,
		.size = 1,
		.description = "Device type (speed/voltage capabilities)",
	},
};

/* Public descriptor functions */
const char *generic_pre_eol_desc(uint8_t value)
{
	switch (value) {
	case 0x00:
		return "Not defined";
	case 0x01:
		return "Normal";
	case 0x02:
		return "Warning";
	case 0x03:
		return "Urgent";
	default:
		return "Reserved";
	}
}

const char *generic_life_time_desc(uint8_t value)
{
	switch (value) {
	case 0x00:
		return "Not defined";
	case 0x01:
		return "0% - 10% used";
	case 0x02:
		return "10% - 20% used";
	case 0x03:
		return "20% - 30% used";
	case 0x04:
		return "30% - 40% used";
	case 0x05:
		return "40% - 50% used";
	case 0x06:
		return "50% - 60% used";
	case 0x07:
		return "60% - 70% used";
	case 0x08:
		return "70% - 80% used";
	case 0x09:
		return "80% - 90% used";
	case 0x0a:
		return "90% - 100% used";
	case 0x0b:
		return "Exceeded lifetime";
	default:
		return "Reserved";
	}
}

const char *generic_ext_csd_rev_desc(uint8_t value)
{
	switch (value) {
	case 5:
		return "eMMC 4.41";
	case 6:
		return "eMMC 4.5";
	case 7:
		return "eMMC 5.0";
	case 8:
		return "eMMC 5.1";
	default:
		return "Unknown";
	}
}

const char *generic_bkops_status_desc(uint8_t value)
{
	switch (value & 0x03) {
	case 0:
		return "No operations";
	case 1:
		return "Non-critical";
	case 2:
		return "Performance impacted";
	case 3:
		return "Critical";
	default:
		return "Unknown";
	}
}

const char *generic_bkops_en_desc(uint8_t value)
{
	if (value & 0x01)
		return "Manual enabled";
	else if (value & 0x02)
		return "Auto enabled";
	else
		return "Disabled";
}

const char *generic_cache_ctrl_desc(uint8_t value)
{
	return (value & 0x01) ? "Enabled" : "Disabled";
}

const char *generic_field_value_desc(unsigned int offset, uint8_t value)
{
	switch (offset) {
	case EXT_CSD_PRE_EOL_INFO:
		return generic_pre_eol_desc(value);
	case EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_A:
	case EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_B:
		return generic_life_time_desc(value);
	case EXT_CSD_REV:
		return generic_ext_csd_rev_desc(value);
	case EXT_CSD_BKOPS_STATUS:
		return generic_bkops_status_desc(value);
	case EXT_CSD_BKOPS_EN:
		return generic_bkops_en_desc(value);
	case EXT_CSD_CACHE_CTRL:
		return generic_cache_ctrl_desc(value);
	default:
		return "";
	}
}

static void print_one_field(const struct smart_device_info *info,
			    const struct ext_csd_field *field, FILE *out)
{
	uint8_t value_u8;
	uint32_t value_u32;
	const char *desc = "";
	char value_str[256] = "";

	if (field->offset + field->size > info->ext_csd_len) {
		fprintf(out, "  %-32s %-45s %s\n", field->name,
			"N/A", field->description);
		return;
	}

	if (field->size == 1) {
		value_u8 = info->ext_csd[field->offset];
		desc = generic_field_value_desc(field->offset, value_u8);

		if (desc[0] != '\0') {
			snprintf(value_str, sizeof(value_str), "0x%02x (%s)", value_u8, desc);
		} else {
			switch (field->offset) {
			case EXT_CSD_BOOT_MULT:
				snprintf(value_str, sizeof(value_str), "0x%02x (%u KB)",
					 value_u8, value_u8 * 128);
				break;
			case EXT_CSD_RPMB_MULT:
				snprintf(value_str, sizeof(value_str), "0x%02x (%u KB)",
					 value_u8, value_u8 * 128);
				break;
			case EXT_CSD_HC_ERASE_GRP_SIZE:
				snprintf(value_str, sizeof(value_str), "0x%02x (%u KB)",
					 value_u8, value_u8 * 512);
				break;
			default:
				snprintf(value_str, sizeof(value_str), "0x%02x", value_u8);
				break;
			}
		}

		fprintf(out, "  %-32s %-45s %s\n", field->name, value_str, field->description);

	} else if (field->size == 4) {
		value_u32 = info->ext_csd[field->offset] |
			    (info->ext_csd[field->offset + 1] << 8) |
			    (info->ext_csd[field->offset + 2] << 16) |
			    (info->ext_csd[field->offset + 3] << 24);

		switch (field->offset) {
		case EXT_CSD_SEC_CNT:
			snprintf(value_str, sizeof(value_str),
				 "0x%08x (%.2f GB)", value_u32, value_u32 * 512.0 / 1e9);
			break;
		case EXT_CSD_CACHE_SIZE:
			snprintf(value_str, sizeof(value_str),
				 "0x%08x (%.2f MB)", value_u32, value_u32 * 512.0 / 1e6);
			break;
		default:
			snprintf(value_str, sizeof(value_str), "0x%08x", value_u32);
			break;
		}

		fprintf(out, "  %-32s %-45s %s\n", field->name, value_str, field->description);

	} else if (field->size == 8) {
		int pos = 0;
		for (unsigned int i = 0; i < 8; i++) {
			pos += snprintf(value_str + pos, sizeof(value_str) - pos,
					"%02x", info->ext_csd[field->offset + i]);
		}
		fprintf(out, "  %-32s %-45s %s\n", field->name, value_str, field->description);
	}
}

void generic_print_fields(const struct smart_device_info *info, FILE *out,
			  const char *vendor_name)
{
	size_t i;

	fprintf(out, "Vendor profile: %s\n", vendor_name);
	fprintf(out, "SMART source  : JEDEC EXT_CSD standard fields\n");
	fprintf(out, "\n");
	fprintf(out, "===============================================================================\n");

	for (i = 0; i < sizeof(generic_fields) / sizeof(generic_fields[0]); i++) {
		const struct ext_csd_field *field = &generic_fields[i];

		/* Print section headers with table header */
		if (i == 0) {
			fprintf(out, "\n[Health Status]\n");
			fprintf(out, "  %-32s %-45s %s\n", "REGISTER", "VALUE", "DESCRIPTION");
			fprintf(out, "  %-32s %-45s %s\n",
				"--------------------------------",
				"---------------------------------------------",
				"----------------------------------------");
		} else if (i == 3) {
			fprintf(out, "\n[Capacity]\n");
			fprintf(out, "  %-32s %-45s %s\n", "REGISTER", "VALUE", "DESCRIPTION");
			fprintf(out, "  %-32s %-45s %s\n",
				"--------------------------------",
				"---------------------------------------------",
				"----------------------------------------");
		} else if (i == 4) {
			fprintf(out, "\n[Background Operations]\n");
			fprintf(out, "  %-32s %-45s %s\n", "REGISTER", "VALUE", "DESCRIPTION");
			fprintf(out, "  %-32s %-45s %s\n",
				"--------------------------------",
				"---------------------------------------------",
				"----------------------------------------");
		} else if (i == 7) {
			fprintf(out, "\n[Cache]\n");
			fprintf(out, "  %-32s %-45s %s\n", "REGISTER", "VALUE", "DESCRIPTION");
			fprintf(out, "  %-32s %-45s %s\n",
				"--------------------------------",
				"---------------------------------------------",
				"----------------------------------------");
		} else if (i == 9) {
			fprintf(out, "\n[Erase Performance]\n");
			fprintf(out, "  %-32s %-45s %s\n", "REGISTER", "VALUE", "DESCRIPTION");
			fprintf(out, "  %-32s %-45s %s\n",
				"--------------------------------",
				"---------------------------------------------",
				"----------------------------------------");
		} else if (i == 12) {
			fprintf(out, "\n[Partition]\n");
			fprintf(out, "  %-32s %-45s %s\n", "REGISTER", "VALUE", "DESCRIPTION");
			fprintf(out, "  %-32s %-45s %s\n",
				"--------------------------------",
				"---------------------------------------------",
				"----------------------------------------");
		} else if (i == 14) {
			fprintf(out, "\n[Performance Features]\n");
			fprintf(out, "  %-32s %-45s %s\n", "REGISTER", "VALUE", "DESCRIPTION");
			fprintf(out, "  %-32s %-45s %s\n",
				"--------------------------------",
				"---------------------------------------------",
				"----------------------------------------");
		} else if (i == 16) {
			fprintf(out, "\n[Firmware & Version]\n");
			fprintf(out, "  %-32s %-45s %s\n", "REGISTER", "VALUE", "DESCRIPTION");
			fprintf(out, "  %-32s %-45s %s\n",
				"--------------------------------",
				"---------------------------------------------",
				"----------------------------------------");
		}

		print_one_field(info, field, out);
	}

	fprintf(out, "\n===============================================================================\n");
}

static int generic_match(const struct smart_device_info *info)
{
	(void)info;
	return 1;  /* Always match as fallback */
}

static void generic_print(const struct smart_device_info *info, FILE *out)
{
	generic_print_fields(info, out, generic_vendor.display_name);

	fprintf(out, "\nNOTE:\n");
	fprintf(out, "  Using generic JEDEC EXT_CSD profile.\n");
	fprintf(out, "  Vendor-specific features are not available.\n");
	fprintf(out, "  Use --vendor option to select a specific vendor profile.\n");
}

const struct smart_vendor generic_vendor = {
	.name = "generic",
	.display_name = "Generic JEDEC eMMC",
	.match = generic_match,
	.print = generic_print,
};
