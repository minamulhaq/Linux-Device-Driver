
#include "asm-generic/int-ll64.h"
#include <linux/dev_printk.h>
#include <linux/device/devres.h>
#include <linux/err.h>
#include <linux/gfp_types.h>
#include <linux/stddef.h>
#include <asm-generic/errno-base.h>
#include <asm/device.h>
#include <linux/compiler.h>
#include <linux/container_of.h>
#include <linux/of.h>
#include <linux/of_device.h>
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
#include <linux/mod_devicetable.h>
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

/* create config struct to used by driver_data field when matching based on ids */
typedef struct {
	int config_item1;
	int config_item2;
} device_config;

enum pcdev_names {
	PCDEVA1X,
	PCDEVB1X,
	PCDEVC1X,
	PCDEVD1X,
};

device_config pcdev_config[] = {
	[PCDEVA1X] = { .config_item1 = 50, .config_item2 = 21 },
	[PCDEVB1X] = { .config_item1 = 51, .config_item2 = 20 },
	[PCDEVC1X] = { .config_item1 = 52, .config_item2 = 19 },
	[PCDEVD1X] = { .config_item1 = 53, .config_item2 = 18 }
};

struct platform_device_id pcdevs_ids[] = {
	{ .name = "pcdev-A1x",
	  /* This driver data is populated by driver 
                            * driver_data: Driver will store some configuration data for device
                            * in this field, so when device is detected, driver Can 
                            * extract this data during probe, when probe is called
                            * you get palatform_device in argument of probe function, 
                            * that device has field platform_device_id*/
	  .driver_data = PCDEVA1X },
	{ .name = "pcdev-B1x", .driver_data = PCDEVB1X },
	{ .name = "pcdev-C1x", .driver_data = PCDEVC1X },
	{ .name = "pcdev-D1x", .driver_data = PCDEVD1X },
	{}
};

struct of_device_id org_pcdev_dt_match[] = {
	{ .compatible = "pcdev-A1x", .data = (void *)PCDEVA1X },
	{ .compatible = "pcdev-B1x", .data = (void *)PCDEVB1X },
	{ .compatible = "pcdev-C1x", .data = (void *)PCDEVC1X },
	{ .compatible = "pcdev-D1x", .data = (void *)PCDEVD1X },
	{}
};
struct platform_driver pcd_platform_driver = {
	.probe = pcd_platform_driver_probe,
	.remove = pcd_platform_driver_remove,
	/* Here we will use of table matching, when matching ids
     * the .driver field below will be ignored 
     */
	// .id_table = pcdevs_ids,
	.driver = { .name = "pseudo-char-device",
		    /* Use this macro to see if CONFIG_OF is enabled in kernel or not i.e. device tree
         * is supported or not */
		    .of_match_table = of_match_ptr(org_pcdev_dt_match) }
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
static pcdev_platform_data *pcdev_get_platform_data_from_dt(struct device *dev)
{
	struct device_node *dev_node = dev->of_node;
	if (!dev_node) {
		/* this probe didn't happen because of device tree node */
		return NULL;
	}

	/* use functions from kernel to extract properties */
	pcdev_platform_data *pdata = (pcdev_platform_data *)devm_kzalloc(
		dev, sizeof(pcdev_platform_data), GFP_KERNEL);
	if (!pdata) {
		dev_info(dev, "Cannot allocate memory\n");
		return ERR_PTR(-ENOMEM);
	}
	if (of_property_read_string(dev_node, "org,device-serial-num",
				    &pdata->serial_number)) {
		dev_info(dev, "Missing serial_number property\n");
		return ERR_PTR(-EINVAL);
	}

	if (of_property_read_u32(dev_node, "org,size", (u32 *)&pdata->size)) {
		dev_info(dev, "Missing size property\n");
		return ERR_PTR(-EINVAL);
	}

	if (of_property_read_u32(dev_node, "org,perm", (u32 *)&pdata->perm)) {
		dev_info(dev, "Missing perm property\n");
		return ERR_PTR(-EINVAL);
	}
	return pdata;
}

/*
 * @brief Single Probe function should support both types of device instantiation either 
 * by adding device manually by calling platform_driver_register that we did from 
 * device setup file or using device tree. 
 */
int pcd_platform_driver_probe(struct platform_device *device)
{
	struct device *dev = &device->dev;
	/* used to store matched entry of 'of_device_id' list of this driver */
	const struct of_device_id *match;
	dev_info(dev, "Device detected\n");

	/* match will always be NULL if LINUX doesn't support device tree i.e.
     * CONFIG_OF is off */
	match = (struct of_device_id *)of_match_device(
		of_match_ptr(org_pcdev_dt_match), dev);

	int ret;
	int driver_data;

	pcdev_platform_data *pdata;
	/*1: Get the platform data either added via manually calling platform_driver_register
     * or from device tree. First check for device tree, if it fails, check for
     * device setup*/

	if (match) {
		/* match happend because of device tree */
		pdata = pcdev_get_platform_data_from_dt(dev);
		if (IS_ERR(pdata)) {
			/* There was problem prcoessing device tree node */
			return PTR_ERR(pdata);
		}
		driver_data = (int)(unsigned long)match->data;
	} else {
		pdata = (pcdev_platform_data *)dev_get_platdata(dev);
		driver_data = (int)(unsigned long)of_device_get_match_data(dev);
	}

	/* if pdata is NULL, device was not instantiated by device tree */
	if (!pdata) {
		dev_info(dev, "No platform data available\n");
		return -EINVAL;
	}

	/*2: Here platform data is available, so we need kernal to Dynamically 
     * allocate memory to store this data
     * Get zeroed out struct
     */

	pcdev_private_data *dev_data;
	dev_data = devm_kzalloc(dev, sizeof(*dev_data), GFP_KERNEL);
	if (!dev_data) {
		dev_info(dev, "Cannot allocate memory\n");
		ret = -ENOMEM;
		return ret;
	}
	/* Here you need to store dev_data in device structure's driver_data field 
     * You can use this device->dev.driver_data = dev_data;
     * or use helper function below
     */

	dev_set_drvdata(dev, dev_data);

	/* copy data from device platform data to zeroed out struct */
	dev_data->pdata.size = pdata->size;
	dev_data->pdata.perm = pdata->perm;
	dev_data->pdata.serial_number = pdata->serial_number;

	dev_info(dev, "Device serial number: %s\n",
		 dev_data->pdata.serial_number);
	dev_info(dev, "Device size: %d\n", dev_data->pdata.size);
	dev_info(dev, "Device permissions: %d\n", dev_data->pdata.perm);
	dev_info(dev, "Device config_item1:  %d\n",
		 pcdev_config[driver_data].config_item1);
	dev_info(dev, "Device config_item2:  %d\n",
		 pcdev_config[driver_data].config_item2);

	/*3: Dynamically allocate memory for the device buffer using size 
     * information from platform data */

	dev_data->buffer =
		devm_kzalloc(&device->dev, dev_data->pdata.size, GFP_KERNEL);
	if (!dev_data->buffer) {
		/* Free previous memory */
		dev_info(dev, "Cannot allocate memory for buffer\n");
		ret = -ENOMEM;
		return ret;
	}

	/*4: Do cdev init and cdev add */
	cdev_init(&dev_data->cdev, &pcd_fops);
	dev_data->cdev.owner = THIS_MODULE;

	/*5: Get device number */
	/* device->id is 0 for first device, 1 for next and so on ... */
	dev_data->dev_num =
		pcdrv_data.device_num_base + pcdrv_data.total_devices;

	ret = cdev_add(&dev_data->cdev, dev_data->dev_num, 1);
	if (ret < 0) {
		pr_err("cdev add failed\n");
		return ret;
	}

	/*6: Create device file for detected platform device */
	pcdrv_data.device_pcd =
		device_create(pcdrv_data.class_pcd, NULL, dev_data->dev_num,
			      NULL, "pcdev-%d", pcdrv_data.total_devices);

	if (IS_ERR(pcdrv_data.device_pcd)) {
		pr_err("Device Create failed\n");
		ret = PTR_ERR(pcdrv_data.device_pcd);
		cdev_del(&dev_data->cdev);
		return ret;
	}

	pcdrv_data.total_devices++;
	return 0;
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

	pcdrv_data.total_devices--;
	dev_info(&device->dev, "A device is removed\n");
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
