#include <linux/module.h>
#include <linux/platform_device.h>
#include "platform.h"

#undef pr_fmt
#define pr_fmt(fmt) "%s :" fmt, __func__

/*1: Create two platfomr data */

pcdev_platform_data pcdev_pdata[2] = {
	[0] = { .size = 512, .perm = RDWR, .serial_number = "PCDEVABC1111" },
	[1] = { .size = 1024, .perm = RDWR, .serial_number = "PCDEVABC2222" }

};

/*2: Create two platfomr devices */
void pcdev_release(struct device *dev);

struct platform_device platform_pcdev_1 = {
	.name = "pseudo-char-device",
	/* ID is used as index */
	.id = 0x00,
	/* Platform data is field of dev property */
	.dev = { .platform_data = &pcdev_pdata[0],
		 /* We need release function when this device is released */
		 .release = pcdev_release }
};
struct platform_device platform_pcdev_2 = {
	.name = "pseudo-char-device",
	.id = 0x01,
	.dev = { .platform_data = &pcdev_pdata[1], .release = pcdev_release }
};

static int __init pcdev_platform_init(void)
{
	/* Register the platform device */
	platform_device_register(&platform_pcdev_1);
	platform_device_register(&platform_pcdev_2);
	pr_info("Device setup module inserted\n");
	return 0;
}

static void __exit pcdev_platform_exit(void)
{
	platform_device_unregister(&platform_pcdev_1);
	platform_device_unregister(&platform_pcdev_2);
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
