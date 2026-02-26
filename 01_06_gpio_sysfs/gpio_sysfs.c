#include <linux/errno.h>
#include <linux/device.h>
#include <linux/stat.h>
#include <linux/stat.h>
#include <linux/sysfs.h>
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
#include <linux/device.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/magic.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/mod_devicetable.h>
#include <linux/gpio/consumer.h>
#include "gpio_sysfs.h"
#include "syscalls.h"

gpiodrv_private_data gpiodrv_data;
gpiodev_private_data d;

/**
 * @brief Return direction of GPIO 
 **/
ssize_t direction_show(struct device *dev, struct device_attribute *attr,
		       char *buf)
{
	/* here we don't use dev->parent because below in device_create_with_groups
     * we passed the dev_data while creating devices */
	gpiodev_private_data *dev_data =
		(gpiodev_private_data *)dev_get_drvdata(dev);
	int dir = gpiod_get_direction(dev_data->desc);
	if (dir < 0) {
		return dir;
	}
	/* if dir = 0, then show "out" if dir =1, then show "in" */
	char *direction;
	direction = (dir == 0) ? "out" : "in";

	return sprintf(buf, "%s\n", direction);
}

ssize_t direction_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	int ret;
	gpiodev_private_data *dev_data =
		(gpiodev_private_data *)dev_get_drvdata(dev);
	// strcmp(buf, "in") we need strcmp but prefer using kernel sysfs function
	if (sysfs_streq(buf, "in")) {
		ret = gpiod_direction_input(dev_data->desc);
	} else if (sysfs_streq(buf, "out")) {
		ret = gpiod_direction_output(dev_data->desc, 1);
	} else {
		ret = -EINVAL;
	}
	return ret ?: count;
}

ssize_t value_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	/* Query the gpio which is in input or output mode */
	int value;
	gpiodev_private_data *dev_data =
		(gpiodev_private_data *)dev_get_drvdata(dev);

	value = gpiod_get_value(dev_data->desc);
	return sprintf(buf, "%d\n", value);
}

ssize_t value_store(struct device *dev, struct device_attribute *attr,
		    const char *buf, size_t count)
{
	/* User is going to send 0 or 1 in form of string */
	int ret;
	gpiodev_private_data *dev_data =
		(gpiodev_private_data *)dev_get_drvdata(dev);

	long value;

	ret = kstrtol(buf, 0, &value);
	if (ret) {
		return ret;
	}
	gpiod_set_value(dev_data->desc, value);
	return count;
}

ssize_t label_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	gpiodev_private_data *dev_data =
		(gpiodev_private_data *)dev_get_drvdata(dev);
	return sprintf(buf, "%s\n", dev_data->label);
}

static DEVICE_ATTR_RW(direction);
static DEVICE_ATTR_RW(value);
static DEVICE_ATTR_RO(label);

static struct attribute *gpio_attrs[] = { &dev_attr_direction.attr,
					  &dev_attr_value.attr,
					  &dev_attr_label.attr, NULL };

static struct attribute_group gpio_attr_group = { .attrs = gpio_attrs };

static const struct attribute_group *gpio_attr_groups[] = { &gpio_attr_group,
							    NULL };

#define driver_compatible_string "org,rpi-gpio-sysfs"

/* List of compatible devices */
struct of_device_id gpio_device_match_ids[] = {
	{ .compatible = driver_compatible_string },
	{}
};

int gpio_sysfs_probe(struct platform_device *pdev)
{
	int ret;
	/* Const char* to extract device label */
	const char *name;

	/* device nodes counter */
	int i = 0;

	/* extract parent */
	struct device_node *parent = pdev->dev.of_node;
	struct device_node *child = NULL;

	/* keep track of child devices */
	gpiodrv_data.total_devices = of_get_child_count(parent);
	if (!gpiodrv_data.total_devices) {
		dev_err(&pdev->dev, "No child devices found\n");
		return -EINVAL;
	}

	dev_info(&pdev->dev, "Child nodes found: %d\n",
		 gpiodrv_data.total_devices);

	/* Allocate memory for child devices */
	gpiodrv_data.dev = devm_kzalloc(&pdev->dev,
					sizeof(struct device *) *
						gpiodrv_data.total_devices,
					GFP_KERNEL);
	if (!gpiodrv_data.dev)
		return -ENOMEM;

	struct device *dev = &pdev->dev;
	/*1: We need to parse gpio from dt, we don't know how many are set
     * We have one parent `gpio_devs` and under it couple of gpios are set. 
     * Driver doesn't know how many gpios are set.  
     */

	/* allocate memory for private data of device */
	gpiodev_private_data *dev_data;
	for_each_available_child_of_node(parent, child) {
		dev_data = devm_kzalloc(dev, sizeof(gpiodev_private_data),
					GFP_KERNEL);
		if (!dev_data) {
			dev_err(dev, "Can't allocate memory\n");
			of_node_put(
				child); /* REQUIRED: Drop ref before early return  */
			return -ENOMEM;
		}

		/* Now we need to read string */

		if (of_property_read_string(child, "label", &name)) {
			dev_warn(dev, "Missing label information\n");
			sprintf(dev_data->label, "Unknown gpio%d", i);
		} else {
			strncpy(dev_data->label, name,
				sizeof(dev_data->label) - 1); /* Safe copy  */
			_dev_info(dev, "GPIO Label: %s\n", dev_data->label);
		}

		dev_data->desc = devm_fwnode_gpiod_get(dev, &child->fwnode,
						       "rpi", GPIOD_ASIS,
						       dev_data->label);

		if (IS_ERR(dev_data->desc)) {
			ret = PTR_ERR(dev_data->desc);
			of_node_put(
				child); /* REQUIRED: Drop ref before early return  */
			return ret;
		}

		/* set gpio direction */
		ret = gpiod_direction_output(dev_data->desc, 0);
		if (ret) {
			_dev_err(dev, "gpio direction set failed\n");
			of_node_put(
				child); /* REQUIRED: Drop ref before early return  */
			return ret;
		}

		/**
         * create devices under /sys/class/rpi_gpios
         * Device is created using device_create(), once device is created, use
         * sysfs functions to create attributes, 
         * However, this can be done in one kernel function device_create_with_groups
         * i. First create null terminated list of attributes.  see gpio_attrs above
         * ii. Create attribute_group, see above gpio_attr_group
         * iii. Now create point array of each attribute_group, see above gpio_attr_groups
         * iv. Create device with groups as below
         **/

		gpiodrv_data.dev[i] = device_create_with_groups(
			gpiodrv_data.class_gpio, dev, 0, dev_data,
			gpio_attr_groups, "%s", (const char *)dev_data->label);
		if (IS_ERR(gpiodrv_data.dev[i])) {
			dev_err(dev, "Error in device_create\n");
			of_node_put(
				child); /* REQUIRED: Drop ref before early return  */
			return PTR_ERR(gpiodrv_data.dev[i]);
		}

		i++;
	}
	/* Update count to actual successfully created devices  */
	gpiodrv_data.total_devices = i;
	return 0;
}

void gpio_sysfs_remove(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "Remove called\n");
	for (unsigned int i = 0; i < gpiodrv_data.total_devices; i++) {
		device_unregister(gpiodrv_data.dev[i]);
	}
	return;
}

struct platform_driver gpiosysfs_platform_driver = {
	.probe = gpio_sysfs_probe,
	.remove = gpio_sysfs_remove,
	.driver = { .name = "rpi-gpiosysfs",
		    .of_match_table = of_match_ptr(gpio_device_match_ids) }
};

int __init gpio_sysfs_init(void)
{
	int ret;

	/*1: Create a class */
	gpiodrv_data.class_gpio = class_create("rpi_gpios");
	if (IS_ERR(gpiodrv_data.class_gpio)) {
		pr_err("Error creating class\r\n");
		return PTR_ERR(gpiodrv_data.class_gpio);
	}

	/*2: Register driver as Platform driver */
	ret = platform_driver_register(&gpiosysfs_platform_driver);
	if (ret) {
		pr_err("Error registering platform driver\n");
		class_destroy(gpiodrv_data.class_gpio);
		return ret;
	}

	pr_info("Module successfully loaded\n");
	return 0;
}

void __exit gpio_sysfs_exit(void)
{
	/*1:  Unregister driver */
	platform_driver_unregister(&gpiosysfs_platform_driver);
	/*2: Destroy the class */
	class_destroy(gpiodrv_data.class_gpio);
}

module_init(gpio_sysfs_init);
module_exit(gpio_sysfs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Muhammad Inam Ul Haq");
MODULE_DESCRIPTION("GPIO-SYSFS Driver");
