#include <linux/module.h>    
#include <linux/fs.h>       
#include <linux/uaccess.h> 
#include <linux/errno.h>  
#include <linux/types.h> 
#include <linux/kernel.h> 
#include "pcd_syscalls.h"

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
