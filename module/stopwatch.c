#include "stopwatch.h"

static int inter_major=242, inter_minor=0;
typedef struct __wait_queue wait_queue_t;
typedef struct __wait_queue_head wait_queue_head_t;

//////       __wake_up(&wait_head,1, 1, NULL);         //////
static unsigned char *iom_fnd_addr;
static int result;
static dev_t inter_dev;
static struct cdev inter_cdev;
static int inter_open(struct inode *, struct file *);
static int inter_release(struct inode *, struct file *);
static int inter_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
static void wait_3sec(void);

irqreturn_t inter_handler1(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t inter_handler2(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t inter_handler3(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t inter_handler4(int irq, void* dev_id, struct pt_regs* reg);

static inter_usage=0;
int interruptCount=0;

/* Wait Queue 쓸것 */
wait_queue_t wait;
wait_queue_head_t wait_head;

/* Start 기능에서 Timer 사용하기 위해 */
struct timer_list timer;
struct timer_list timer2;

unsigned long int paused=0;

unsigned long int step_in;
unsigned long int step_in_mk;
int handler4_flag = 0;


unsigned char value[4]; // for FND device
int start; // for FND device

int start_flag;
int paused_flag=0;
int exit_flag;

static struct file_operations inter_fops =
{
	.owner = THIS_MODULE,
	.open = inter_open,
	.write = inter_write,
	.release = inter_release,
};

static void fnd_init(void){

    unsigned short int value_short = 0;
   // unsigned char zero = 0;
   // value_short = zero << 12 | zero << 8 |zero << 4 |zero;
    outw(value_short,(unsigned int)iom_fnd_addr);
     printk("Initializing fnd device...!!\n");
}

/*  Home 버튼에 대한 interrupt handler     */
irqreturn_t inter_handler1(int irq, void* dev_id, struct pt_regs* reg) {
	printk(KERN_ALERT "interrupt1!!! = %x\n", gpio_get_value(IMX_GPIO_NR(1, 11)));
	start_flag = 1; 

	timer.expires = get_jiffies_64() + (10 * (HZ/10)); // delay: 1~100..
    timer.function = loop;
	printk("Before\n");
    add_timer(&timer);
	printk("After\n");
	return IRQ_HANDLED;
}


/*  Back 버튼에 대한 interrupt handler     */
irqreturn_t inter_handler2(int irq, void* dev_id, struct pt_regs* reg) { 
		
        printk(KERN_ALERT "interrupt2!!! = %x\n", gpio_get_value(IMX_GPIO_NR(1, 12)));
		start_flag = 0;
		paused_flag ^= 1;
		

		if(paused_flag == 0){
			printk("paused time: %ums\n",paused);
			del_timer_sync(&timer);
			timer.expires = get_jiffies_64() + (100-paused); // delay: 1~100..
    		timer.function = loop;
			start_flag = 1;
			add_timer(&timer);
		}

		else if(paused_flag ==1){
			paused = get_jiffies_64();
			printk("paused val: %d\n",paused);
			paused -= step_in_mk;
			printk("paused time: %ums\n",paused);
			del_timer_sync(&timer);
		}
		
        return IRQ_HANDLED;
}


/*  Vol+ 버튼에 대한 interrupt handler     */
irqreturn_t inter_handler3(int irq, void* dev_id,struct pt_regs* reg) {
        printk(KERN_ALERT "interrupt3!!! = %x\n", gpio_get_value(IMX_GPIO_NR(2, 15)));
		if(start_flag==1){
			start = -1;
		}
		else if(paused_flag ==1){
			start = 0;
			fnd_init();
		}

        return IRQ_HANDLED;
}


/*  Vol- 버튼에 대한 interrupt handler   */
irqreturn_t inter_handler4(int irq, void* dev_id, struct pt_regs* reg) {
        printk(KERN_ALERT "interrupt4!!! = %x\n", gpio_get_value(IMX_GPIO_NR(5, 14)));
		if(handler4_flag==0){
		handler4_flag = 1;
		timer2.expires = get_jiffies_64() + (30 * (HZ/10));
		timer2.function = wait_3sec;
		add_timer(&timer2);
		}
        return IRQ_HANDLED;
}

void wait_3sec(void){
	
	int ret;
	
	ret = gpio_get_value(IMX_GPIO_NR(5, 14));
	handler4_flag = 0;
	if(!ret){
		__wake_up(&wait_head,1, 1, NULL);
	}

}

static int inter_open(struct inode *minode, struct file *mfile){
	int ret1, ret2, ret3, ret4;
	int irq;

	printk(KERN_ALERT "Open Module\n");


	iom_fnd_addr = ioremap(FND_ADDRESS, 0x4);   // for FND device
	start = 0;
	init_timer(&timer);
	init_timer(&timer2);
	fnd_init();  // 다시 한 번 초기화

	// int1
	gpio_direction_input(IMX_GPIO_NR(1,11));     // IMX_GPIO_NR(1, 11) signal from home button will be used as an input.
	irq = gpio_to_irq(IMX_GPIO_NR(1,11));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret1=request_irq(irq, inter_handler1, IRQF_DISABLED | IRQF_TRIGGER_FALLING, "/dev/stopwatch", 0);

	// int2
	gpio_direction_input(IMX_GPIO_NR(1,12));
	irq = gpio_to_irq(IMX_GPIO_NR(1,12));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret2=request_irq(irq, inter_handler2, IRQF_DISABLED | IRQF_TRIGGER_FALLING, "/dev/stopwatch", 0);
	//ret=request_irq();

	// int3
	gpio_direction_input(IMX_GPIO_NR(2,15));
	irq = gpio_to_irq(IMX_GPIO_NR(2,15));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret3=request_irq(irq, inter_handler3, IRQF_DISABLED | IRQF_TRIGGER_FALLING, "/dev/stopwatch", 0);
	//ret=request_irq();

	// int4
	gpio_direction_input(IMX_GPIO_NR(5,14));
	irq = gpio_to_irq(IMX_GPIO_NR(5,14));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret4=request_irq(irq, inter_handler4, IRQF_DISABLED | IRQF_TRIGGER_FALLING, "/dev/stopwatch", 0);
	//ret=request_irq();

	return 0;
}

static int inter_release(struct inode *minode, struct file *mfile){
	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 11)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 12)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(2, 15)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(5, 14)), NULL);

	fnd_init(); // 다시 한 번 초기화
	iounmap(iom_fnd_addr);
	del_timer_sync(&timer);
	del_timer_sync(&timer2);

	printk(KERN_ALERT "Release Module\n");
	return 0;
}

static void make_time(int time_calc){
	
	int min;
	int sec;
	unsigned short int value_short = 0;

	min = time_calc / 60;
	sec = time_calc % 60;

	value[0] = min / 10;
	value[1] = min % 10;
	value[2] = sec / 10;
	value[3] = sec % 10;

    value_short = value[0] << 12 | value[1] << 8 |value[2] << 4 |value[3];
    outw(value_short,(unsigned int)iom_fnd_addr);	    

}

static void loop(void){

	int i;
	int tmp;
	
	exit_flag = 0;

	if(start_flag == 1){
		start++; // start_flag가 1일 경우에만 시간 증가
	}


	make_time(start);
	

    printk("Timer deleted\n");
	
    timer.expires = get_jiffies_64() + (10 * (HZ/10)); // delay: 1~100..
    timer.function = loop;

    add_timer(&timer);
	
	step_in_mk = get_jiffies_64();
	printk("step_in_mk is %d\n",step_in_mk);

    printk("Timer added\n");

}

static int inter_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos ){
	printk("write\n");

	init_waitqueue_head(&wait_head);
	interruptible_sleep_on (&wait_head);


	return 0;
}

static int inter_register_cdev(void)
{
	int error;
	if(inter_major) {
		inter_dev = MKDEV(inter_major, inter_minor);
		error = register_chrdev_region(inter_dev,1,"stopwatch");
	}else{
		error = alloc_chrdev_region(&inter_dev,inter_minor,1,"stopwatch");
		inter_major = MAJOR(inter_dev);
	}
	if(error<0) {
		printk(KERN_WARNING "inter: can't get major %d\n", inter_major);
		return result;
	}
	printk(KERN_ALERT "major number = %d\n", inter_major);
	cdev_init(&inter_cdev, &inter_fops);
	inter_cdev.owner = THIS_MODULE;
	inter_cdev.ops = &inter_fops;
	error = cdev_add(&inter_cdev, inter_dev, 1);
	if(error)
	{
		printk(KERN_NOTICE "inter Register Error %d\n", error);
	}
	return 0;
}

static int __init inter_init(void) {
	int result;
	if((result = inter_register_cdev()) < 0 )
		return result;

	
	printk(KERN_ALERT "Init Module Success \n");
	printk(KERN_ALERT "Device : /dev/stopwatch, Major Num : 242 \n");
	return 0;
}

static void __exit inter_exit(void) {
	cdev_del(&inter_cdev);
	unregister_chrdev_region(inter_dev, 1);
	
	
	printk(KERN_ALERT "Remove Module Success \n");
}

module_init(inter_init);
module_exit(inter_exit);
	MODULE_LICENSE("GPL");
	MODULE_AUTHOR("You");
