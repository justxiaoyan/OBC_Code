#include "longsys.h"
#include "../generic/generic.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int contains_casefold(const char *haystack, const char *needle)
{
	size_t needle_len;

	if (!haystack || !needle)
		return 0;

	needle_len = strlen(needle);
	if (needle_len == 0)
		return 1;

	for (; *haystack; haystack++) {
		size_t i;

		for (i = 0; i < needle_len; i++) {
			unsigned char h = (unsigned char)haystack[i];
			unsigned char n = (unsigned char)needle[i];

			if (!h)
				return 0;
			if (tolower(h) != tolower(n))
				break;
		}
		if (i == needle_len)
			return 1;
	}

	return 0;
}

static int longsys_match(const struct smart_device_info *info)
{
	if (!info)
		return 0;

	return contains_casefold(info->sysfs_name, "longsys") ||
	       contains_casefold(info->sysfs_name, "foresee") ||
	       contains_casefold(info->sysfs_name, "jiangbolong");
}

static void longsys_print(const struct smart_device_info *info, FILE *out)
{
	/* Use generic print function */
	generic_print_fields(info, out, longsys_vendor.display_name);

	/* Add Longsys-specific notes */
	fprintf(out, "\nNOTE:\n");
	fprintf(out, "  Write Amplification Index (WAI) requires vendor-specific commands.\n");
	fprintf(out, "  Standard JEDEC EXT_CSD does not provide host/NAND write statistics.\n");
	fprintf(out, "  Contact Longsys for vendor-specific command documentation.\n");
	fprintf(out, "\n");
	fprintf(out, "  Longsys vendor-specific features:\n");
	fprintf(out, "    - Currently using standard JEDEC fields only\n");
	fprintf(out, "    - Vendor commands: Not implemented (require documentation)\n");
}

const struct smart_vendor longsys_vendor = {
	.name = "longsys",
	.display_name = "Jiangbolong/Longsys eMMC",
	.match = longsys_match,
	.print = longsys_print,
};
