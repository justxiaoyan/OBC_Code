// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2000-2009
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

#include <bootm.h>
#include <command.h>
#include <image.h>
#include <irq_func.h>
#include <lmb.h>
#include <log.h>
#include <linux/compiler.h>
#include <fdtdec.h>
#include <fdt.h>


int __weak bootz_setup(ulong image, ulong *start, ulong *end)
{
	/* Please define bootz_setup() for your platform */

	puts("Your platform's zImage format isn't supported yet!\n");
	return -1;
}

/*
 * zImage booting support
 */
static int bootz_start(struct cmd_tbl *cmdtp, int flag, int argc,
		       char *const argv[], struct bootm_headers *images)
{
	ulong zi_start, zi_end;
	struct bootm_info bmi;
	int ret;

	bootm_init(&bmi);
	if (argc)
		bmi.addr_img = argv[0];
	if (argc > 1)
		bmi.conf_ramdisk = argv[1];
	if (argc > 2)
		bmi.conf_fdt = argv[2];
	/* do not set up argc and argv[] since nothing uses them */

	ret = bootm_run_states(&bmi, BOOTM_STATE_START);

	/* Setup Linux kernel zImage entry point */
	if (!argc) {
		images->ep = image_load_addr;
		debug("*  kernel: default image load address = 0x%08lx\n",
				image_load_addr);
	} else {
		images->ep = hextoul(argv[0], NULL);
		debug("*  kernel: cmdline image address = 0x%08lx\n",
			images->ep);
	}

	ret = bootz_setup(images->ep, &zi_start, &zi_end);
	if (ret != 0)
		return 1;

	lmb_reserve(&images->lmb, images->ep, zi_end - zi_start);

	/*
	 * Handle the BOOTM_STATE_FINDOTHER state ourselves as we do not
	 * have a header that provide this informaiton.
	 */
	if (bootm_find_images(image_load_addr, cmd_arg1(argc, argv),
			      cmd_arg2(argc, argv), images->ep,
			      zi_end - zi_start))
		return 1;

	return 0;
}

void parse_partitions(const void *fdt, int node_offset) {
   int subnode_offset;
   int len;
   const char *label;
   const fdt32_t *reg;
   int reg_len;

   // 遍历 partitions 节点中的所有子节点
   for (subnode_offset = fdt_first_subnode(fdt, node_offset);
        subnode_offset >= 0;
        subnode_offset = fdt_next_subnode(fdt, subnode_offset)) {
       // 获取 label 属性
       label = fdt_getprop(fdt, subnode_offset, "label", &len);
       if (label) {
           printf("Label: %s\n", label);
       }

       // 获取 reg 属性
       reg = fdt_getprop(fdt, subnode_offset, "reg", &reg_len);
       if (reg) {
           uint32_t addr = fdt32_to_cpu(reg[0]); // 32-bit address
           uint32_t size = fdt32_to_cpu(reg[1]); // 32-bit size
           printf("  Address: 0x%x, Size: 0x%x\n", addr, size);
       }
   }
}

int find_usdhc2_node(const void *fdt, int node_offset) {
   const char *compatible;
   int len;

  // 检查当前节点是否是 &usdhc2
  compatible = fdt_getprop(fdt, node_offset, "obcpart", &len);
  if (compatible && (strstr(compatible, "obc-emmc"))) {
      return node_offset;
  }

   // 递归查找子节点
   int subnode_offset;
   for (subnode_offset = fdt_first_subnode(fdt, node_offset);
        subnode_offset >= 0;
        subnode_offset = fdt_next_subnode(fdt, subnode_offset)) {
       int found_node = find_usdhc2_node(fdt, subnode_offset);
       if (found_node >= 0) {
           return found_node;
       }
   }

   return -1;
}

void parse_device_tree(const void *fdt) {
   int node_offset;
   int partitions_node;

   // 从根节点开始查找 &usdhc2 节点
   node_offset = find_usdhc2_node(fdt, 0);
   if (node_offset < 0) {
       printf("Error: usdhc2 node not found\n");
       return;
   }

   // 打印找到的节点信息
   const char *node_name = fdt_get_name(fdt, node_offset, NULL);
   if (node_name) {
       printf("Found usdhc2 node: %s\n", node_name);
   } else {
       printf("Error: Unable to get name of usdhc2 node\n");
       return;
   }

   // 查找 partitions 节点
   partitions_node = fdt_subnode_offset(fdt, node_offset, "partitions");
   if (partitions_node < 0) {
       printf("Error: partitions node not found\n");

       // 打印 &usdhc2 节点的所有子节点，以确认 partitions 节点是否存在
       printf("Listing all subnodes of usdhc2 node:\n");
       int subnode_offset;
       for (subnode_offset = fdt_first_subnode(fdt, node_offset);
            subnode_offset >= 0;
            subnode_offset = fdt_next_subnode(fdt, subnode_offset)) {
           const char *subnode_name = fdt_get_name(fdt, subnode_offset, NULL);
           if (subnode_name) {
               printf("  Subnode: %s\n", subnode_name);
           } else {
               printf("  Error: Unable to get name of subnode\n");
           }
       }
       return;
   }

   // 打印 partitions 节点信息
   const char *partitions_name = fdt_get_name(fdt, partitions_node, NULL);
   if (partitions_name) {
       printf("Found partitions node: %s\n", partitions_name);
   } else {
       printf("Error: Unable to get name of partitions node\n");
       return;
   }

   // 解析 partitions 节点
   parse_partitions(fdt, partitions_node);
}


int do_bootz(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	struct bootm_info bmi;
	int ret;

	/* Consume 'bootz' */
	argc--; argv++;

	if (bootz_start(cmdtp, flag, argc, argv, &images))
		return 1;

	/*
	 * We are doing the BOOTM_STATE_LOADOS state ourselves, so must
	 * disable interrupts ourselves
	 */
	bootm_disable_interrupts();

	images.os.os = IH_OS_LINUX;

	bootm_init(&bmi);
	if (argc)
		bmi.addr_img = argv[0];
	if (argc > 1)
		bmi.conf_ramdisk = argv[1];
	if (argc > 2)
		bmi.conf_fdt = argv[2];
	bmi.cmd_name = "bootz";

    const void *fdt_addr = (void *)0x83000000; // 假设设备树位于 0x83000000

    // 检查设备树是否有效
    if (fdt_check_header(fdt_addr) != 0) {
        printf("Error: Invalid device tree header\n");
        return -1;
    }

    // 解析设备树
    parse_device_tree(fdt_addr);


	ret = bootz_run(&bmi);

	return ret;
}

U_BOOT_LONGHELP(bootz,
	"[addr [initrd[:size]] [fdt]]\n"
	"    - boot Linux zImage stored in memory\n"
	"\tThe argument 'initrd' is optional and specifies the address\n"
	"\tof the initrd in memory. The optional argument ':size' allows\n"
	"\tspecifying the size of RAW initrd.\n"
#if defined(CONFIG_OF_LIBFDT)
	"\tWhen booting a Linux kernel which requires a flat device-tree\n"
	"\ta third argument is required which is the address of the\n"
	"\tdevice-tree blob. To boot that kernel without an initrd image,\n"
	"\tuse a '-' for the second argument. If you do not pass a third\n"
	"\ta bd_info struct will be passed instead\n"
#endif
	);

U_BOOT_CMD(
	bootz,	CONFIG_SYS_MAXARGS,	1,	do_bootz,
	"boot Linux zImage image from memory", bootz_help_text
);
