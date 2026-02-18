
#ifndef PLATFORM_H
#define PLATFORM_H

#define RDONLY 0x01
#define WRONLY 0x10
#define RDWR 0x11

/* Platform data for our device */
typedef struct {
	int size;
	int perm;
	const char *serial_number;
} pcdev_platform_data;
#endif
