#include "asm-generic/errno-base.h"
#include "linux/compiler.h"
#include "linux/device/class.h"
#include "linux/err.h"
#include "linux/init.h"
#include "linux/magic.h"
#include "linux/printk.h"
#include "linux/string.h"
#include "linux/types.h"
#include "linux/uaccess.h"
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>

#undef pr_fmt
#define pr_fmt(fmt) "%s :" fmt, __func__

#define DEV_MEM_SIZE 512

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

/* Add this function before chardev_init() */
static char *mydevice_devnode(const struct device *dev, umode_t *mode)
{
	if (mode)
		*mode = 0666; /* rw-rw-rw- */
	return NULL;
}

static int __init chardev_init(void)
{
	int ret;
	/*1. Dynamically allocate a device number 
     * Defined in <linus/fs.h>*/
	ret = alloc_chrdev_region(&device_number, 0, 1, "mydevicesnumbers");
	if (ret < 0) {
		pr_err("allocation failed\n");
		goto out;
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
		/* goto are allowed in linux kernel programming */
		goto delete_cdev;
	}

	myclass->devnode = mydevice_devnode;

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
out:
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

ssize_t my_read(struct file *filep, char __user *buff, size_t count,
		loff_t *f_pos)
{
	pr_info("Read is requested for %zu bytes\n", count);
	pr_info("current file position = %lld\n", *f_pos);

	/* Adjust the count for read */
	if ((*f_pos + count) > DEV_MEM_SIZE) {
		count = DEV_MEM_SIZE - *f_pos;
	}

	/* Copy to User */
	if (copy_to_user(buff, &device_buffer[*f_pos], count)) {
		return -EFAULT;
	}

	/* Update Current file position */
	*f_pos += count;
	pr_info("Number of bytes sucessfully read: %zu\n", count);
	pr_info("updated file position = : %lld\n", *f_pos);

	/* return number of bytes read */
	return count;
}
ssize_t my_write(struct file *filep, const char __user *buff, size_t count,
		 loff_t *f_pos)
{
	pr_info("Write is requested for %zu bytes\n", count);
	pr_info("current file position = %lld\n", *f_pos);

	/* Adjust the count for read */
	if ((*f_pos + count) > DEV_MEM_SIZE) {
		count = DEV_MEM_SIZE - *f_pos;
	}

	/* If there is nothing to write */
	if (!count) {
		pr_err("No space left on device\n");
		return -ENOMEM;
	}

	/* Copy from User */
	if (copy_from_user(&device_buffer[*f_pos], buff, count)) {
		return -EFAULT;
	}

	/* Update Current file position */
	*f_pos += count;
	pr_info("Number of bytes sucessfully written: %zu\n", count);
	pr_info("updated file position = : %lld\n", *f_pos);

	/* return number of bytes read */
	return count;
}

loff_t my_llseek(struct file *filep, loff_t offset, int whence)
{
	pr_info("lseek requested\n");
	pr_info("current file position = %lld\n", filep->f_pos);
	loff_t temp;

	switch (whence) {
	case SEEK_SET: {
		if ((offset > DEV_MEM_SIZE) || (offset < 0)) {
			return -EINVAL;
		}
		filep->f_pos = offset;
		break;
	}
	case SEEK_CUR: {
		temp = filep->f_pos + offset;
		if ((temp > DEV_MEM_SIZE) || (temp < 0)) {
			return -EINVAL;
		}
		filep->f_pos = temp;
		break;
	}

	case SEEK_END: {
		temp = DEV_MEM_SIZE + offset;
		if ((temp < 0) || (temp > DEV_MEM_SIZE)) {
			return -EINVAL;
		}
		filep->f_pos = temp;
		break;
	}
	default: {
		return -EINVAL;
	}
	}
	pr_info("Updated file position = %lld\n", filep->f_pos);
	return filep->f_pos;
}

int my_open(struct inode *inode, struct file *filep)
{
	pr_info("Open was successfull\n");
	return 0;
}
int my_release(struct inode *inode, struct file *filep)
{
	pr_info("release was successfull\n");
	return 0;
}

module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Muhammad Inam Ul Haq");
MODULE_DESCRIPTION("Simple Chardev Module");
