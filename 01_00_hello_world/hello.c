// Include required headers
#include <linux/init.h>
#include <linux/module.h>

/*
 * We need to register two callbacks
 * init callback -> when module is loaded
 * exit callback -> when module is removed
 */

// Should return 0 if modules is loaded successfully
static int myinit(void) {
  printk("Hello World!!!\n");
  return 0;
}

static void myexit(void) { printk("Module removed!!!\n"); }


// Register Callbacks
module_init(myinit);
module_exit(myexit);

MODULE_LICENSE("GPL");
