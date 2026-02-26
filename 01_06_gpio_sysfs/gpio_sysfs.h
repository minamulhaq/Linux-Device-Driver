#ifndef SYSFS_GPIO_SYSFS_H__
#define SYSFS_GPIO_SYSFS_H__

#include <linux/platform_device.h>

#undef pr_fmt
#define pr_fmt(fmt) "%s :" fmt, __func__

#define RDONLY 0x01
#define WRONLY 0x10
#define RDWR 0x11

/* Device private data structure */
typedef struct {
	char label[20];
	struct gpio_desc *desc;
} gpiodev_private_data;

/* Driver Private data */
typedef struct {
	int total_devices;
	struct class *class_gpio;
    /* We use double pointer to store the devices dynamically */
	struct device **dev;
} gpiodrv_private_data;

int __init gpio_sysfs_init(void);
void __exit gpio_sysfs_exit(void);

int gpio_sysfs_probe(struct platform_device *pdev);
void gpio_sysfs_remove(struct platform_device *pdev);

ssize_t direction_show(struct device *dev, struct device_attribute *attr,
		       char *buf);

ssize_t direction_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count);

ssize_t value_show(struct device *dev, struct device_attribute *attr,
		   char *buf);

ssize_t value_store(struct device *dev, struct device_attribute *attr,
		    const char *buf, size_t count);

ssize_t label_show(struct device *dev, struct device_attribute *attr,
		   char *buf);

#endif
