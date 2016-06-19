#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/ioctl.h>
#include <linux/delay.h>
#include <linux/gpio.h>

#include <mach/gpio.h>
#include <mach/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <mach/board-revision.h>


#define DEVICE_NAME "leds"

static int led_gpios[] = {
	S3C2410_GPB(5),
	S3C2410_GPB(6),
	S3C2410_GPA(25),
	S3C2410_GPA(26),
};

#define LED_NUM		ARRAY_SIZE(led_gpios)


static long mini2451_leds_ioctl(struct file *filp, unsigned int cmd,
		unsigned long arg)
{
	switch(cmd) {
		case 0:
		case 1:
			if (arg > LED_NUM) {
				return -EINVAL;
			}

			gpio_set_value(led_gpios[arg], !cmd);
			//printk(DEVICE_NAME": %d %d\n", arg, cmd);
			break;

		default:
			return -EINVAL;
	}

	return 0;
}

static struct file_operations mini2451_led_dev_fops = {
	.owner			= THIS_MODULE,
	.unlocked_ioctl	= mini2451_leds_ioctl,
};

static struct miscdevice mini2451_led_dev = {
	.minor			= MISC_DYNAMIC_MINOR,
	.name			= DEVICE_NAME,
	.fops			= &mini2451_led_dev_fops,
};

static int __init mini2451_led_dev_init(void) {
	int ret;
	int i;

	if (is_board_rev_B()) {
		led_gpios[2] = S3C2410_GPB(7);
		led_gpios[3] = S3C2410_GPB(8);
	}

	for (i = 0; i < LED_NUM; i++) {
		ret = gpio_request(led_gpios[i], "LED");
		if (ret) {
			printk("%s: request GPIO %d for LED failed, ret = %d\n", DEVICE_NAME,
					led_gpios[i], ret);
			return ret;
		}

		s3c_gpio_cfgpin(led_gpios[i], S3C_GPIO_OUTPUT);
		gpio_set_value(led_gpios[i], 1);
	}

	ret = misc_register(&mini2451_led_dev);

	printk(DEVICE_NAME"\tinitialized\n");

	return ret;
}

static void __exit mini2451_led_dev_exit(void) {
	int i;

	for (i = 0; i < LED_NUM; i++) {
		gpio_free(led_gpios[i]);
	}

	misc_deregister(&mini2451_led_dev);
}

module_init(mini2451_led_dev_init);
module_exit(mini2451_led_dev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("FriendlyARM Inc.");

