#include "linux/device/class.h"
#include "linux/err.h"
#include "linux/init.h"
#include "linux/printk.h"
#include "linux/string.h"
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>

#undef pr_fmt
#define pr_fmt(fmt) "%s :" fmt, __func__

#define DEV_MEM_SIZE 256

/* device Memory */
char device_buffer[DEV_MEM_SIZE];

/* device number */
dev_t device_number;

/* Cdev variables */
struct cdev my_cdev;

loff_t my_llseek(struct file *filep, loff_t off, int whence);
ssize_t my_read(struct file *filep, char __user *buff, size_t count,
		loff_t *f_pos);
ssize_t my_write(struct file *filep, const char __user *buff, size_t count,
		 loff_t *f_pos);
int my_open(struct inode *inode, struct file *filep);
int my_release(struct inode *inode, struct file *filep);
/* file operations for driver */
struct file_operations my_fops = { .llseek = my_llseek,
				   .read = my_read,
				   .write = my_write,
				   .open = my_open,
				   .release = my_release };

struct class *myclass = NULL;
struct device *device_mydevice = NULL;

static int __init chardev_init(void)
{
	int ret;
	/*1. Dynamically allocate a device number 
     * Defined in <linus/fs.h>*/
	ret = alloc_chrdev_region(&device_number, 0, 1, "mydevicesnumbers");
	if (ret < 0) {
		pr_err("allocation failed\n");
		return ret;
	}

	/*2. For each device, create 1 cdev struct and one file_operations struct
     * cdev_init defined in fs/char_dev.c
     * Note: cdev_init first clears out the cdev struct, don't initialize anything before INIT !!!
     */
	cdev_init(&my_cdev, &my_fops);

	/*3. Register device/structure with Kernal using cdev_add */
	my_cdev.owner = THIS_MODULE;
	ret = cdev_add(&my_cdev, device_number, 1);
	if (ret < 0) {
		goto unregister_region;
	}

	/*4. Create device class under /sys/class/ */
	myclass = class_create("my_char_class");
	if (IS_ERR(myclass)) {
		ret = PTR_ERR(myclass);
		goto delete_cdev;
	}

	/*5. popluate the sysfs with device information */
	device_mydevice = device_create(myclass, NULL, device_number, NULL,
					"mydriverdevice");
	if (IS_ERR(device_mydevice)) {
		ret = PTR_ERR(device_mydevice);
		goto destroy_class;
	}
	pr_info("Module init successfull\n");
	pr_info("Device Number <major>:<minor> = %d:%d\n", MAJOR(device_number),
		MINOR(device_number));

	return 0;
destroy_class:
	class_destroy(myclass);
delete_cdev:
	cdev_del(&my_cdev);
unregister_region:
	unregister_chrdev_region(device_number, 1);
	return ret;
}

static void __exit chardev_exit(void)
{
	/* cleanup is done in reverse order of creation */
	device_destroy(myclass, device_number);
	class_destroy(myclass);
	cdev_del(&my_cdev);
	unregister_chrdev_region(device_number, 1);
	pr_info("module unloaded\n");
}

loff_t my_llseek(struct file *filep, loff_t off, int whence)
{
	return 0;
}
ssize_t my_read(struct file *filep, char __user *buff, size_t count,
		loff_t *f_pos)
{
	return 0;
}
ssize_t my_write(struct file *filep, const char __user *buff, size_t count,
		 loff_t *f_pos)
{
	return 0;
}
int my_open(struct inode *inode, struct file *filep)
{
	return 0;
}
int my_release(struct inode *inode, struct file *filep)
{
	return 0;
}

module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Muhammad Inam Ul Haq");
MODULE_DESCRIPTION("Simple Chardev Module");
