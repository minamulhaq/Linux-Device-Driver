#include "asm-generic/errno-base.h"
#include "linux/compiler.h"
#include "linux/container_of.h"
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

#define MAX_MEM_SIZE_DEV1 512
#define MAX_MEM_SIZE_DEV2 512
#define MAX_MEM_SIZE_DEV3 512
#define MAX_MEM_SIZE_DEV4 512

/* device Memory */
char device_buffer_dev1[MAX_MEM_SIZE_DEV1];
char device_buffer_dev2[MAX_MEM_SIZE_DEV2];
char device_buffer_dev3[MAX_MEM_SIZE_DEV3];
char device_buffer_dev4[MAX_MEM_SIZE_DEV4];

#define NO_DEVICES 4

/* Device private data structure */
typedef struct {
	char *buffer;
	unsigned int size;
	const char *serial_number;
	int perm;
	struct cdev cdev;
} device_private_data;

/* Driver private data structure */
typedef struct {
	int total_devices;
	/* device number */
	dev_t device_number;
	device_private_data dpd[NO_DEVICES];
	struct class *myclass;
	struct device *device_mydevice;
} driver_private_data;

#define RONLY 0x01
#define WONLY 0x10
#define RDWR 0x11

driver_private_data drvpvtdata = {
	.total_devices = NO_DEVICES,
	.dpd = { {
			 .buffer = device_buffer_dev1,
			 .size = MAX_MEM_SIZE_DEV1,
			 .serial_number = "char_device1",
			 .perm = RONLY, /* RDONLY */
		 },
		 {
			 .buffer = device_buffer_dev2,
			 .size = MAX_MEM_SIZE_DEV2,
			 .serial_number = "char_device2",
			 .perm = WONLY, /* WRONLY */
		 },
		 {
			 .buffer = device_buffer_dev3,
			 .size = MAX_MEM_SIZE_DEV3,
			 .serial_number = "char_device3",
			 .perm = RDWR, /* RDWR*/
		 },
		 {
			 .buffer = device_buffer_dev4,
			 .size = MAX_MEM_SIZE_DEV4,
			 .serial_number = "char_device4",
			 .perm = RDWR, /* RDWR */
		 } },

};

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
	unsigned int i;

	ret = alloc_chrdev_region(&drvpvtdata.device_number, 0, NO_DEVICES,
				  "mydevices");
	if (ret < 0) {
		pr_err("allocation failed\n");
		return ret; 
	}

	/* Create device class under /sys/class/ */
	drvpvtdata.myclass = class_create("my_char_class");
	if (IS_ERR(drvpvtdata.myclass)) {
		ret = PTR_ERR(drvpvtdata.myclass);
		goto unregister_region; 
	}

	drvpvtdata.myclass->devnode = mydevice_devnode; 

	for (i = 0; i < NO_DEVICES; i++) {
		pr_info("Device number <major>:<minor> = %d:%d\n",
			MAJOR(drvpvtdata.device_number + i),
			MINOR(drvpvtdata.device_number + i));

		cdev_init(&drvpvtdata.dpd[i].cdev, &my_fops); 
		drvpvtdata.dpd[i].cdev.owner = THIS_MODULE;

		ret = cdev_add(&drvpvtdata.dpd[i].cdev,
			       drvpvtdata.device_number + i, 1);
		if (ret < 0) {
			pr_err("Char device %d add failed\n", i);
			goto cleanup_devices; 
		}

		drvpvtdata.device_mydevice = device_create(
			drvpvtdata.myclass, NULL,
			drvpvtdata.device_number + i, 
			NULL, "device-%d", i + 1);
		if (IS_ERR(drvpvtdata.device_mydevice)) {
			ret = PTR_ERR(drvpvtdata.device_mydevice);
			cdev_del(
				&drvpvtdata.dpd[i].cdev); 
			goto cleanup_devices; 
		}
	}

	pr_info("Module init successful\n");
	return 0;

cleanup_devices:
	/* Clean up devices created so far (0 to i-1) */
	for (; i > 0; i--) {
		device_destroy(drvpvtdata.myclass,
			       drvpvtdata.device_number + i - 1); 
		cdev_del(&drvpvtdata.dpd[i - 1].cdev); 
	}
	class_destroy(drvpvtdata.myclass);
unregister_region:
	unregister_chrdev_region(drvpvtdata.device_number, NO_DEVICES);
	return ret;
}

static void __exit chardev_exit(void)
{
	int i;
	for (i = 0; i < NO_DEVICES; i++) {
		device_destroy(drvpvtdata.myclass,
			       drvpvtdata.device_number + i);
		cdev_del(&drvpvtdata.dpd[i].cdev);
	}
	class_destroy(drvpvtdata.myclass);
	unregister_chrdev_region(drvpvtdata.device_number, NO_DEVICES);
}

ssize_t my_read(struct file *filep, char __user *buff, size_t count,
		loff_t *f_pos)
{
	pr_info("Read is requested for %zu bytes\n", count);
	pr_info("current file position = %lld\n", *f_pos);

	/* in filep we already store the file private_data 
	 * Adjust the count for read 
     */

	device_private_data *dev_data =
		(device_private_data *)filep->private_data;

	if ((*f_pos + count) > dev_data->size) {
		count = dev_data->size - *f_pos;
	}

	/* Copy to User */
	if (copy_to_user(buff, dev_data->buffer + (*f_pos), count)) {
		// if (copy_to_user(buff, &device_buffer[*f_pos], count)) {
		return -EFAULT;
	}

	/* Update Current file position */
	*f_pos += count;
	pr_info("Number of bytes sucessfully read: %zu\n", count);
	pr_info("updated file position = : %lld\n", *f_pos);

	/* return number of bytes read */
	return count;
	return 0;
}
ssize_t my_write(struct file *filep, const char __user *buff, size_t count,
		 loff_t *f_pos)
{
	pr_info("Write is requested for %zu bytes\n", count);
	pr_info("current file position = %lld\n", *f_pos);

	device_private_data *dev_data =
		(device_private_data *)filep->private_data;
	int DEV_MEM_SIZE = dev_data->size;

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
	if (copy_from_user(dev_data->buffer + (*f_pos), buff, count)) {
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

	device_private_data *dev_data =
		(device_private_data *)filep->private_data;
	int DEV_MEM_SIZE = dev_data->size;

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

static int check_permissions(int perm, fmode_t mode)
{
	if (perm == RDWR) {
		return 0;
	}

	/* If mode is read only and file is open for read and not for write */
	if ((perm == RONLY) && (mode & FMODE_READ) && !(mode & FMODE_WRITE)) {
		return 0;
	}

	/* If mode is write only and file is open for write and not for read */
	if ((perm == WONLY) && (mode & FMODE_WRITE) && !(mode & FMODE_READ)) {
		return 0;
	}

	return -EPERM;
}

int my_open(struct inode *inode, struct file *filep)
{
	int ret;
	/* Driver must find out on which device file the open was attmepted by the user space from INODE */

	int minor_num = MINOR(inode->i_rdev);
	pr_info("Minor access: %d\n", minor_num);

	/* get device prive data structure using container_of macro in linux.h*,
     * store information in filep so this kernel object is passed to other 
     * functions like read, write etc because they don't receive inode, they get
     * file pointer 
     */
	device_private_data *dev_data = (device_private_data *)container_of(
		inode->i_cdev, device_private_data, cdev);

	filep->private_data = dev_data;

	//    /* check permissions */
	ret = check_permissions(dev_data->perm, filep->f_mode);
	(!ret) ? pr_info("Open was successfull\n") :
		 pr_info("Open was unsuccessful\n");
	return ret;
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
MODULE_DESCRIPTION("Simple Chardev Module that handles multiple char devices");
