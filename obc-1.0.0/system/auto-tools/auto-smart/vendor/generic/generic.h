#ifndef GENERIC_H
#define GENERIC_H

#include "../smart_vendor.h"

extern const struct smart_vendor generic_vendor;

/* Common field descriptors */
const char *generic_pre_eol_desc(uint8_t value);
const char *generic_life_time_desc(uint8_t value);
const char *generic_ext_csd_rev_desc(uint8_t value);
const char *generic_bkops_status_desc(uint8_t value);
const char *generic_bkops_en_desc(uint8_t value);
const char *generic_cache_ctrl_desc(uint8_t value);

/* Common field value descriptor */
const char *generic_field_value_desc(unsigned int offset, uint8_t value);

/* Common print function */
void generic_print_fields(const struct smart_device_info *info, FILE *out,
			  const char *vendor_name);

#endif
