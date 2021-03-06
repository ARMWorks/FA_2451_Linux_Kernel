#include <linux/kernel.h>
#include <linux/module.h>


static int __init mini2451_hello_module_init(void)
{
    printk("Hello, Mini6410 module is installed !\n");
    return 0;
}

static void __exit mini2451_hello_module_cleanup(void)
{
    printk("Good-bye, Mini6410 module was removed!\n");
}

module_init(mini2451_hello_module_init);
module_exit(mini2451_hello_module_cleanup);
MODULE_LICENSE("GPL");
