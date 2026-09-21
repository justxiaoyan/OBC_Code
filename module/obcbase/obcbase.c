// SPDX-License-Identifier: GPL-2.0

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/string.h>

struct obc_boot_info {
	char media[8];
	u32 media_id;
	u32 boot_part;
	char device[16];
};

#define OBC_BOOTARGS_SIZE 2048

static struct obc_boot_info obc_boot;

static int obc_get_bootarg(const char *bootargs, const char *key,
			   char *value, size_t value_size)
{
	const char *cursor = bootargs;
	size_t key_len = strlen(key);

	while (*cursor) {
		const char *end;
		const char *equals;
		size_t value_len;

		while (*cursor == ' ')
			cursor++;
		if (!*cursor)
			break;

		end = strchr(cursor, ' ');
		if (!end)
			end = cursor + strlen(cursor);
		equals = memchr(cursor, '=', end - cursor);
		if (equals && equals - cursor == key_len &&
		    !strncmp(cursor, key, key_len)) {
			value_len = end - equals - 1;
			if (!value_len || value_len >= value_size)
				return -EINVAL;
			memcpy(value, equals + 1, value_len);
			value[value_len] = '\0';
			return 0;
		}

		cursor = end;
	}

	return -ENOENT;
}

static int obc_parse_bootargs(const char *bootargs)
{
	char part[8];
	char media[8];
	unsigned int part_number;
	int ret;

	ret = obc_get_bootarg(bootargs, "obcboot.part", part, sizeof(part));
	if (ret)
		return ret;
	ret = obc_get_bootarg(bootargs, "obcboot.media", media, sizeof(media));
	if (ret)
		return ret;

	if (kstrtouint(part, 10, &part_number) || part_number > 1)
		return -EINVAL;
	if (strcmp(media, "emmc") && strcmp(media, "sd"))
		return -EINVAL;

	strscpy(obc_boot.media, media, sizeof(obc_boot.media));
	obc_boot.media_id = !strcmp(media, "sd");
	obc_boot.boot_part = part_number;
	snprintf(obc_boot.device, sizeof(obc_boot.device), "mmcblk%u",
		 obc_boot.media_id);
	return 0;
}

/*
 * U-Boot publishes the selected slot and media in bootargs.  The kernel
 * exposes the final command line through /proc/cmdline, so this module does
 * not need to inspect the DT or duplicate the partition table maintained by
 * mdev in /dev/obcblock.
 */
static int obc_read_bootargs(void)
{
	struct file *file;
	loff_t offset = 0;
	char *bootargs;
	ssize_t length;
	int ret;

	bootargs = kmalloc(OBC_BOOTARGS_SIZE, GFP_KERNEL);
	if (!bootargs)
		return -ENOMEM;

	file = filp_open("/proc/cmdline", O_RDONLY, 0);
	if (IS_ERR(file)) {
		ret = PTR_ERR(file);
		goto out_free;
	}

	length = kernel_read(file, bootargs, OBC_BOOTARGS_SIZE - 1, &offset);
	filp_close(file, NULL);
	if (length < 0) {
		ret = (int)length;
		goto out_free;
	}
	bootargs[length] = '\0';
	strreplace(bootargs, '\n', ' ');

	ret = obc_parse_bootargs(bootargs);

out_free:
	kfree(bootargs);
	return ret;
}

static int obc_block_show(struct seq_file *file, void *unused)
{
	seq_printf(file, "boot_device=%s\n", obc_boot.device);
	seq_puts(file, "partition_links=/dev/obcblock\n");
	seq_puts(file, "source=kernel PARTNAME events handled by mdev\n");
	return 0;
}

static int obc_sysinfo_show(struct seq_file *file, void *unused)
{
	seq_printf(file, "boot_media=%s\n", obc_boot.media);
	seq_printf(file, "media_id=%u\n", obc_boot.media_id);
	seq_printf(file, "boot_part=%u\n", obc_boot.boot_part);
	seq_printf(file, "boot_device=%s\n", obc_boot.device);
	return 0;
}

static int obc_sysinfo_part_show(struct seq_file *file, void *unused)
{
	seq_printf(file, "%u\n", obc_boot.boot_part);
	return 0;
}

static int __init obcbase_init(void)
{
	struct proc_dir_entry *proc_dir;
	int ret;

	ret = obc_read_bootargs();
	if (ret) {
		pr_err("obcbase: missing or invalid obcboot.part/media bootargs\n");
		return ret;
	}

	proc_dir = proc_mkdir("obcbase", NULL);
	if (!proc_dir)
		return -ENOMEM;

	if (!proc_create_single("block", 0444, proc_dir, obc_block_show) ||
	    !proc_create_single("sysinfo", 0444, proc_dir, obc_sysinfo_show) ||
	    !proc_create_single("sysinfo_part", 0444, proc_dir,
				obc_sysinfo_part_show)) {
		remove_proc_subtree("obcbase", NULL);
		return -ENOMEM;
	}

	pr_info("obcbase: %s slot %u from bootargs\n", obc_boot.media,
		obc_boot.boot_part);
	return 0;
}

static void __exit obcbase_exit(void)
{
	remove_proc_subtree("obcbase", NULL);
}

module_init(obcbase_init);
module_exit(obcbase_exit);

MODULE_AUTHOR("OBC Project");
MODULE_DESCRIPTION("OBC boot source information driver");
MODULE_LICENSE("GPL");
