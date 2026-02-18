#ifndef PRIVATE_DATA_H
#define PRIVATE_DATA_H

#include <linux/device.h>
#include <linux/types.h>
#include "platform.h"


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
#endif
