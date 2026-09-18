#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "my_device"

// 设备号
static int major_num;

// 文件操作结构体
static struct file_operations fops = {
    .owner = THIS_MODULE,
    // 这里可以添加你需要的文件操作函数指针
    // .open = my_open,
    // .read = my_read,
    // .write = my_write,
    // .release = my_release,
};

// 模块初始化函数
static int __init my_module_init(void)
{
    // 注册字符设备
    major_num = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_num < 0) {
        printk(KERN_ALERT "Failed to register device: %d\n", major_num);
        return major_num;
    }
    
    printk(KERN_INFO "Module loaded. Major number: %d\n", major_num);
    return 0;
}

// 模块退出函数
static void __exit my_module_exit(void)
{
    // 注销字符设备
    unregister_chrdev(major_num, DEVICE_NAME);
    printk(KERN_INFO "Module unloaded.\n");
}

// 模块宏定义
module_init(my_module_init);
module_exit(my_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple Linux driver module");
