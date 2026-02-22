#ifndef PCD_SYSCALLS_H
#define PCD_SYSCALLS_H
#include <linux/types.h>
#include <linux/fs.h>

loff_t pcd_lseek(struct file *filep, loff_t off, int whence);
ssize_t pcd_read(struct file *filep, char __user *buff, size_t count,
		 loff_t *f_pos);
ssize_t pcd_write(struct file *filep, const char __user *buff, size_t count,
		  loff_t *f_pos);
int pcd_open(struct inode *inode, struct file *filep);
int pcd_release(struct inode *inode, struct file *filep);

#endif
