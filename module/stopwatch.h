#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <mach/gpio.h>
#include <linux/platform_device.h>
#include <asm/gpio.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/init.h>
#include<linux/delay.h>
#include <linux/ioport.h>
#include <linux/version.h>
#include <linux/cdev.h>

#include<linux/types.h>
#include <linux/ioctl.h>
#include<linux/timer.h>

#include <linux/slab.h>

#include <linux/uaccess.h>
#include <linux/io.h>


#define DEV_MAJOR 242
#define DEV_NAME "stopwatch"
#define FND_ADDRESS 0x08000004


static void fnd_init(void);
static void loop(void);
static void make_time(int);
static void stop(void); // 일시정지 버튼 
static void reset(void); //리셋 버튼
