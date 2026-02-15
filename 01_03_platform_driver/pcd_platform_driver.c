
#include <asm-generic/errno-base.h>
#include <asm/device.h>
#include <linux/compiler.h>
#include <linux/container_of.h>
#include <linux/init.h>
#include <linux/magic.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include "platform.h"
#include "private_data.h"

#undef pr_fmt
#define pr_fmt(fmt) "%s :" fmt, __func__

/* One global variable for driver private data strcuture */
pcdrv_private_data pcdrv_data;

loff_t pcd_lseek(struct file *filep, loff_t off, int whence);
ssize_t pcd_read(struct file *filep, char __user *buff, size_t count,
		 loff_t *f_pos);
ssize_t pcd_write(struct file *filep, const char __user *buff, size_t count,
		  loff_t *f_pos);
int pcd_open(struct inode *inode, struct file *filep);
int pcd_release(struct inode *inode, struct file *filep);
/* file operations for driver */
struct file_operations pcd_fops = { .read = pcd_read,
				    .write = pcd_write,
				    .llseek = pcd_lseek,
				    .open = pcd_open,
				    .release = pcd_release,
				    .owner = THIS_MODULE };

/* we need to initialize platform driver
 * we need to initialize the name field of driver field of platform_driver
 * The matching mechanicsm matches the name field for driver with name field of device
 * if match is successfull, probe function of driver is called
 */

/* Called when matched platform device is found */
int pcd_platform_driver_probe(struct platform_device *device);

/* Called when matched platform device is removed */
void pcd_platform_driver_remove(struct platform_device *device);

struct platform_driver pcd_platform_driver = {
	.probe = pcd_platform_driver_probe,
	.remove = pcd_platform_driver_remove,
	.driver = { .name = "pseudo-char-device" }
};

static int __init pcd_platform_driver_init(void)
{
	/* This is one time initialization unlike probe which is called multiple times */
	int ret;
	/*1: Dynamically allocate a device number for MAX_DEVICES */

	ret = alloc_chrdev_region(&pcdrv_data.device_num_base, 0, MAX_DEVICES,
				  "pcdevs");
	if (ret < 0) {
		pr_err("allocation chardev failed\n");
		return ret;
	}

	/*2: Create device class */
	pcdrv_data.class_pcd = class_create("pcd_class");
	if (IS_ERR(pcdrv_data.class_pcd)) {
		pr_err("Failed to create class\n");
		ret = PTR_ERR(pcdrv_data.class_pcd);
		unregister_chrdev_region(pcdrv_data.device_num_base,
					 MAX_DEVICES);
		return ret;
	}

	/*3: Register platform driver */
	/* Should check for errors and rollback in case of error */
	platform_driver_register(&pcd_platform_driver);
	pr_info("pcd platform driver loaded\n");
	return 0;
}

static void __exit pcd_platform_driver_exit(void)
{
	/*1: Unregister the platform driver */
	platform_driver_unregister(&pcd_platform_driver);

	/*2: Destroy the Class */
	class_destroy(pcdrv_data.class_pcd);

	/*3: Unregister device numbers for MAX_DEVICES */
	unregister_chrdev_region(pcdrv_data.device_num_base, MAX_DEVICES);
	pr_info("pcd platform driver unloaded\n");
}

int pcd_platform_driver_probe(struct platform_device *device)
{
	pr_info("Device detected\n");
	int ret;
	/*1: Get the platform data */

	/* extract platform data  in temprorary varaible */
	pcdev_platform_data *pdata;
	pdata = (pcdev_platform_data *)dev_get_platdata(&device->dev);
	if (!pdata) {
		/* No platform data found */
		pr_info("No platform data available\n");
		ret = -EINVAL;
		goto out;
	}

	/*2: Here platform data is available, so we need kernal to Dynamically 
     * allocate memory to store this data
     * Get zeroed out struct
     */

	pcdev_private_data *dev_data;
	dev_data = kzalloc(sizeof(*dev_data), GFP_KERNEL);
	if (!dev_data) {
		pr_info("Cannot allocate memory\n");
		ret = -ENOMEM;
		goto out;
	}
	/* Here you need to store dev_data in device structure's driver_data field 
     * You can use this device->dev.driver_data = dev_data;
     * or use helper function below
     */

	dev_set_drvdata(&device->dev, dev_data);

	/* copy data from device platform data to zeroed out struct */
	dev_data->pdata.size = pdata->size;
	dev_data->pdata.perm = pdata->perm;
	dev_data->pdata.serial_number = pdata->serial_number;

	pr_info("Device serial number: %s\n", dev_data->pdata.serial_number);
	pr_info("Device size: %d\n", dev_data->pdata.size);
	pr_info("Device permissions: %d\n", dev_data->pdata.perm);

	/*3: Dynamically allocate memory for the device buffer using size 
     * information from platform data */

	dev_data->buffer = kzalloc(sizeof(dev_data->pdata.size), GFP_KERNEL);
	if (!dev_data->buffer) {
		/* Free previous memory */
		pr_info("Cannot allocate memory for buffer\n");
		ret = -ENOMEM;
		goto dev_data_free;
	}

	/*4: Do cdev init and cdev add */
	cdev_init(&dev_data->cdev, &pcd_fops);
	dev_data->cdev.owner = THIS_MODULE;

	/*5: Get device number */
	/* device->id is 0 for first device, 1 for next and so on ... */
	dev_data->dev_num = pcdrv_data.device_num_base + device->id;

	ret = cdev_add(&dev_data->cdev, dev_data->dev_num, 1);
	if (ret < 0) {
		pr_err("cdev add failed\n");
		goto buffer_free;
	}


	/*6: Create device file for detected platform device */
	pcdrv_data.device_pcd = device_create(pcdrv_data.class_pcd, NULL,
					      dev_data->dev_num, NULL,
					      "pcdev-%d", device->id);
	if (IS_ERR(pcdrv_data.device_pcd)) {
		pr_err("Device Create failed\n");
		ret = PTR_ERR(pcdrv_data.device_pcd);
		goto cdev_delete;
	}

	pcdrv_data.total_devices++;
	pr_info("Probe was succcessfule\n");
	return 0;
/* Error Handling */
cdev_delete:
	cdev_del(&dev_data->cdev);
buffer_free:
	kfree(dev_data->buffer);
dev_data_free:
	kfree(dev_data);
out:
	pr_info("Device Probe failed\n");
	return ret;
}
void pcd_platform_driver_remove(struct platform_device *device)
{
	/*1: Remove device that was created with device_create
     * Here we get the device but how to get driver data? device has field 
     * driver_data that contains the driver data held by device */

	/* Extract device private data */
	pcdev_private_data *dev_data = dev_get_drvdata(&device->dev);
	device_destroy(pcdrv_data.class_pcd, dev_data->dev_num);

	/*2: Remove cdev entry from system */
	cdev_del(&dev_data->cdev);

	/*3: Free the memory held by device */
	kfree(dev_data->buffer);
	kfree(dev_data);
	pcdrv_data.total_devices--;
	pr_info("A device is removed\n");
}

ssize_t pcd_read(struct file *filep, char __user *buff, size_t count,
		 loff_t *f_pos)
{
	return 0;
}
ssize_t pcd_write(struct file *filep, const char __user *buff, size_t count,
		  loff_t *f_pos)
{
	return -ENOMEM;
}

loff_t pcd_lseek(struct file *filep, loff_t offset, int whence)
{
	return 0;
}

static int check_permissions(int perm, fmode_t mode)
{
	if (perm == RDWR) {
		return 0;
	}

	/* If mode is read only and file is open for read and not for write */
	if ((perm == RDONLY) && (mode & FMODE_READ) && !(mode & FMODE_WRITE)) {
		return 0;
	}

	/* If mode is write only and file is open for write and not for read */
	if ((perm == WRONLY) && (mode & FMODE_WRITE) && !(mode & FMODE_READ)) {
		return 0;
	}

	return -EPERM;
}

int pcd_open(struct inode *inode, struct file *filep)
{
	return 0;
}
int pcd_release(struct inode *inode, struct file *filep)
{
	pr_info("release was successfull\n");
	return 0;
}

module_init(pcd_platform_driver_init);
module_exit(pcd_platform_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Muhammad Inam Ul Haq");
MODULE_DESCRIPTION(
	"A pseudo platform driver which handles n platform pcdevices");
