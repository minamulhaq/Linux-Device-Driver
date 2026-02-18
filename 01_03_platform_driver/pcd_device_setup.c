#include <linux/array_size.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include "platform.h"

#undef pr_fmt
#define pr_fmt(fmt) "%s :" fmt, __func__

/*1: Create two platfomr data */

pcdev_platform_data pcdev_pdata[] = {
	{ .size = 512, .perm = RDWR, .serial_number = "PCDEVABC1111" },
	{ .size = 1024, .perm = RDWR, .serial_number = "PCDEVABC2222" },
	{ .size = 512, .perm = RDWR, .serial_number = "PCDEVABC3333" },
	{ .size = 2048, .perm = RDWR, .serial_number = "PCDEVABC4444" }
};

/*2: Create two platfomr devices */
void pcdev_release(struct device *dev);

struct platform_device platform_pcdev_1 = {
	.name = "pcdev-A1x",
	/* ID is used as index */
	.id = 0x00,
	/* Platform data is field of dev property */
	.dev = { .platform_data = &pcdev_pdata[0],
		 /* We need release function when this device is released */
		 .release = pcdev_release }
};
struct platform_device platform_pcdev_2 = {
	.name = "pcdev-B1x",
	.id = 0x01,
	.dev = { .platform_data = &pcdev_pdata[1], .release = pcdev_release }
};

struct platform_device platform_pcdev_3 = {
	.name = "pcdev-C1x",
	.id = 0x02,
	.dev = { .platform_data = &pcdev_pdata[2], .release = pcdev_release }
};

struct platform_device platform_pcdev_4 = {
	.name = "pcdev-D1x",
	.id = 0x03,
	.dev = { .platform_data = &pcdev_pdata[3], .release = pcdev_release }
};

struct platform_device *pc_devs[] = {
	&platform_pcdev_1,
	&platform_pcdev_2,
	&platform_pcdev_3,
	&platform_pcdev_4,
};

static int __init pcdev_platform_init(void)
{
	/* Register the platform device
     * You can register devices 1 by 1 like below
	platform_device_register(&platform_pcdev_1);
	platform_device_register(&platform_pcdev_2);
     */

	platform_add_devices(pc_devs, ARRAY_SIZE(pc_devs));
	pr_info("Device setup module inserted\n");
	return 0;
}

static void __exit pcdev_platform_exit(void)
{
	platform_device_unregister(&platform_pcdev_1);
	platform_device_unregister(&platform_pcdev_2);
	platform_device_unregister(&platform_pcdev_3);
	platform_device_unregister(&platform_pcdev_4);
	pr_info("Device setup module unloaded\n");
}

void pcdev_release(struct device *dev)
{
	pr_info("Device released\n");
}

module_init(pcdev_platform_init);
module_exit(pcdev_platform_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Platform driver registration");
