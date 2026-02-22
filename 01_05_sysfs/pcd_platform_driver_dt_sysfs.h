#ifndef PCD_PLATFORM_DRIVER_DT_SYSFS_H
#define PCD_PLATFORM_DRIVER_DT_SYSFS_H

#include <linux/platform_device.h>

#undef pr_fmt
#define pr_fmt(fmt) "%s :" fmt, __func__

#define RDONLY 0x01
#define WRONLY 0x10
#define RDWR 0x11

/* Platform data for our device */
typedef struct {
	int size;
	int perm;
	const char *serial_number;
} pcdev_platform_data;

#define MAX_DEVICES 10

/* Device Private Data */
typedef struct {
	pcdev_platform_data pdata;
	char *buffer;
	dev_t dev_num;
	struct cdev cdev;
} pcdev_private_data;

/* Driver private data structure */

typedef struct {
	int total_devices;
	dev_t device_num_base;
	struct class *class_pcd;
	struct device *device_pcd;
} pcdrv_private_data;

int pcd_platform_driver_probe(struct platform_device *device);
void pcd_platform_driver_remove(struct platform_device *device);

#endif
