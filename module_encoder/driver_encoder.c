#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
 
#include <linux/interrupt.h>
#include <linux/gpio.h>

#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <asm/uaccess.h>
 
 
#define DRIVER_AUTHOR "Mehmet Akcakoca <mehmet.akcakoca@inovasyonmuhendislik.com>"
#define DRIVER_DESC   "Evarobot Encoder Driver"
#define DEVICE_NAME   "evarobotEncoder"
 
// we want GPIO_17 (pin 11 on P5 pinout raspberry pi rev. 2 board)
// to generate interrupt
#define GPIO_PINA0      4
#define GPIO_PINB0		17
#define GPIO_PINA1      27 
#define GPIO_PINB1		22
 
// text below will be seen in 'cat /proc/interrupt' command
#define GPIO_PINA0_DESC          "gpioa0 interrupt"
#define GPIO_PINA1_DESC          "gpioa1 interrupt"

#define GPIO_PINB0_DESC          "gpiob0 interrupt"
#define GPIO_PINB1_DESC          "gpiob1 interrupt"
 
// below is optional, used in more complex code, in our case, this could be
// NULL
#define GPIO_PINA0_DEVICE_DESC    "gpioa0"
#define GPIO_PINA1_DEVICE_DESC    "gpioa1" 

#define GPIO_PINB0_DEVICE_DESC    "gpiob0"
#define GPIO_PINB1_DEVICE_DESC    "gpiob1" 

/****************************************************************************/
/* Driver variables                                                         */
/****************************************************************************/
struct fake_device{
	char data[100];
	struct semaphore sem;
}virtual_device;

struct cdev *mcdev;
int major_number;
int ret;

dev_t dev_num;

/****************************************************************************/
/* Encoder variables                                                        */
/****************************************************************************/
short int directionL = 1, directionR = 1;
long durationR = 0, durationL = 0;
 
 
/****************************************************************************/
/* Interrupts variables block                                               */
/****************************************************************************/
short int irq_pina0    = 0;
short int irq_pina1    = 0;

short int irq_pinb0    = 0;
short int irq_pinb1    = 0;

 
/****************************************************************************/
/* IRQ handler - fired on interrupt                                         */
/****************************************************************************/
static irqreturn_t r_irq_handler(int irq, void *dev_id, struct pt_regs *regs) {
 
   unsigned long flags;
   int a_val;
   int b_val;

   // disable hard interrupts (remember them in flag 'flags')
   local_irq_save(flags);

  if(irq == irq_pina0){
	// GPIO0 Interrupt
	
	// Read B1
	// static inline int gpio_get_value(unsigned int gpio)
	b_val = gpio_get_value(GPIO_PINB0);
	a_val = gpio_get_value(GPIO_PINA0);

	if((a_val == 1 && b_val == 0) || (a_val == 0 && b_val == 1))
		directionL =  0;
	else if((a_val == 1 && b_val == 1) || (a_val == 0 && b_val == 0))
		directionL = 1;
	else
		printk(KERN_NOTICE "evarobotEncoder: Undefined b0 a0 inta0\n");

	if(directionL == 0)
		durationL--;
	else
		durationL++;

  }
  else if(irq == irq_pina1){
	// GPIO1 Interrupt

        // Read B2
        // static inline int gpio_get_value(unsigned int gpio)
        b_val = gpio_get_value(GPIO_PINB1);
        a_val = gpio_get_value(GPIO_PINA1);

        if((a_val == 1 && b_val == 0) || (a_val == 0 && b_val == 1))
                directionR =  0;
        else if((a_val == 1 && b_val == 1) || (a_val == 0 && b_val == 0))
                directionR = 1;
        else
                printk(KERN_NOTICE "evarobotEncoder: Undefined b1 a1 inta1\n");



        if(directionR == 1)
                durationR--;
        else
                durationR++;


  }
  else if(irq == irq_pinb0){
        // GPIO1 Interrupt

        // Read B2
        // static inline int gpio_get_value(unsigned int gpio)
        b_val = gpio_get_value(GPIO_PINB0);
        a_val = gpio_get_value(GPIO_PINA0);

        if((a_val == 1 && b_val == 1) || (a_val == 0 && b_val == 0))
                directionL =  0;
        else if((a_val == 0 && b_val == 1) || (a_val == 1 && b_val == 0))
                directionL = 1;
        else
               printk(KERN_NOTICE "evarobotEncoder: Undefined b0 a0 intb0\n");


        if(directionL == 0)
                durationL--;
        else
                durationL++;


  }
  else if(irq == irq_pinb1){
        // GPIO1 Interrupt

        // Read B2
        // static inline int gpio_get_value(unsigned int gpio)
        b_val = gpio_get_value(GPIO_PINB1);
        a_val = gpio_get_value(GPIO_PINA1);

        if((a_val == 1 && b_val == 1) || (a_val == 0 && b_val == 0))
                directionR =  0;
        else if((a_val == 0 && b_val == 1) || (a_val == 1 && b_val == 0))
                directionR = 1;
        else
                printk(KERN_NOTICE "evarobotEncoder: Undefined b1 a1 intb1\n");


        if(directionR == 1)
                durationR--;
        else
                durationR++;


  }
  else{
    printk(KERN_NOTICE "evarobotEncoder: Undefined interrupt\n");
  }

 

//  printk(KERN_NOTICE "DurationL = %d", durationL);
//  printk(KERN_NOTICE "DurationR = %d", durationR);

   // NOTE:
   // Anonymous Sep 17, 2013, 3:16:00 PM:
   // You are putting printk while interupt are disabled. printk can block.
   // It's not a good practice.
   // 
   // hardware.coder:
   // http://stackoverflow.com/questions/8738951/printk-inside-an-interrupt-handler-is-it-really-that-bad
//   printk(KERN_NOTICE "Interrupt Flag: %lu", flags);
 //  printk(KERN_NOTICE "Interrupt [%d] for device %s was triggered !.\n",
 //         irq, (char *) dev_id);
 
   // restore hard interrupts
   local_irq_restore(flags);
 
   return IRQ_HANDLED;
}
 
 
/****************************************************************************/
/* This function configures interrupts.                                     */
/****************************************************************************/
void r_int_config(void) {
 
   if (gpio_request(GPIO_PINA0, GPIO_PINA0_DESC)) {
      printk("evarobotEncoder: GPIO request faiure: %s\n", GPIO_PINA0_DESC);
      return;
   }
 
   if ( (irq_pina0 = gpio_to_irq(GPIO_PINA0)) < 0 ) {
      printk("evarobotEncoder: GPIO to IRQ mapping faiure %s\n", GPIO_PINA0_DESC);
      return;
   }

   if (gpio_request(GPIO_PINA1, GPIO_PINA1_DESC)) {
      printk("evarobotEncoder: GPIO request faiure: %s\n", GPIO_PINA1_DESC);
      return;
   }
 
   if ( (irq_pina1 = gpio_to_irq(GPIO_PINA1)) < 0 ) {
      printk("evarobotEncoder: GPIO to IRQ mapping faiure %s\n", GPIO_PINA1_DESC);
      return;
   }

   if (gpio_request(GPIO_PINB0, GPIO_PINB0_DESC)) {
      printk("evarobotEncoder: GPIO request faiure: %s\n", GPIO_PINB0_DESC);
      return;
   }
 
   if ( (irq_pinb0 = gpio_to_irq(GPIO_PINB0)) < 0 ) {
      printk("evarobotEncoder: GPIO to IRQ mapping faiure %s\n", GPIO_PINB0_DESC);
      return;
   }
 
   if (gpio_request(GPIO_PINB1, GPIO_PINB1_DESC)) {
      printk("evarobotEncoder: GPIO request faiure: %s\n", GPIO_PINB1_DESC);
      return;
   }
 
   if ( (irq_pinb1 = gpio_to_irq(GPIO_PINB1)) < 0 ) {
      printk("evarobotEncoder: GPIO to IRQ mapping faiure %s\n", GPIO_PINB1_DESC);
      return;
   }


   printk(KERN_NOTICE "evarobotEncoder: Mapped int %d\n", irq_pina0);
   printk(KERN_NOTICE "evarobotEncoder: Mapped int %d\n", irq_pina1); 

   printk(KERN_NOTICE "evarobotEncoder: Mapped int %d\n", irq_pinb0);
   printk(KERN_NOTICE "evarobotEncoder: Mapped int %d\n", irq_pinb1); 

   if (request_irq(irq_pina0,
                   (irq_handler_t ) r_irq_handler,
//                   IRQF_TRIGGER_RISING,
		   IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
                   GPIO_PINA0_DESC,
                   GPIO_PINA0_DEVICE_DESC)) {
      printk("evarobotEncoder: A0 Irq Request failure\n");
      return;
   }

   if (request_irq(irq_pina1,
                   (irq_handler_t ) r_irq_handler,
//                   IRQF_TRIGGER_RISING,
		   IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
                   GPIO_PINA1_DESC,
                   GPIO_PINA1_DEVICE_DESC)) {
      printk("evarobotEncoder: A1 Irq Request failure\n");
      return;
   }

   if (request_irq(irq_pinb0,
                   (irq_handler_t ) r_irq_handler,
//                   IRQF_TRIGGER_RISING,
		   IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
                   GPIO_PINB0_DESC,
                   GPIO_PINB0_DEVICE_DESC)) {
      printk("evarobotEncoder: B0 Irq Request failure\n");
      return;
   }

   if (request_irq(irq_pinb1,
                   (irq_handler_t ) r_irq_handler,
//                   IRQF_TRIGGER_RISING,
		   IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
                   GPIO_PINB1_DESC,
                   GPIO_PINB1_DEVICE_DESC)) {
      printk("evarobotEncoder: B1 Irq Request failure\n");
      return;
   }


 
   return;
}

/****************************************************************************/
/* This functions inits encoders.                                             */
/****************************************************************************/
void encoder_init(void){
	directionL = 1;
	directionR = 1;

	durationL = 0;
	durationR = 0;

	// PinB1 as input
	// static inline int gpio_export(unsigned gpio, bool direction_may_change)
//	gpio_export(GPIO_PINB0, true);

	// PinB2 as input
//	gpio_export(GPIO_PINB1, true);

	return;
}

 
 
/****************************************************************************/
/* This function releases interrupts.                                       */
/****************************************************************************/
void r_int_release(void) {
 
   free_irq(irq_pina0, GPIO_PINA0_DEVICE_DESC);
   gpio_free(GPIO_PINA0);

   free_irq(irq_pina1, GPIO_PINA1_DEVICE_DESC);
   gpio_free(GPIO_PINA1);
   
   free_irq(irq_pinb0, GPIO_PINB0_DEVICE_DESC);
   gpio_free(GPIO_PINB0);

   free_irq(irq_pinb1, GPIO_PINB1_DEVICE_DESC);
   gpio_free(GPIO_PINB1);

//   gpio_unexport(GPIO_PINB0);
//   gpio_unexport(GPIO_PINB1);

 
   return;
}
 
/****************************************************************************/
/* This function opens the device 				                            */
/****************************************************************************/ 
int device_open(struct inode *inode, struct file *filp){

	if(down_interruptible(&virtual_device.sem) != 0){
		printk(KERN_ALERT "evarobotEncoder: could not lock device during open");
		return -1;
	}

	printk(KERN_INFO "evarobotEncoder: opened device");
	return 0;
}

/****************************************************************************/
/* This function reads from the device.                                     */
/****************************************************************************/
ssize_t device_read(struct file* filp, char* bufStoreData, size_t bufCount, loff_t* curOffset){

//	printk(KERN_INFO "evarobotEncoder: reading from device L: %li , R: %li \n", durationL, durationR);
	
	sprintf( virtual_device.data, "%li_%li", durationL, durationR);
	ret = copy_to_user(bufStoreData, virtual_device.data, bufCount);
	return ret;
}


/****************************************************************************/
/* This function writes to the device.                                      */
/****************************************************************************/
ssize_t device_write(struct file* filp, const char* bufSourceData, size_t bufCount, loff_t* curOffset){
//	printk(KERN_INFO "evarobotEncoder: writing to device");
	//ret = copy_from_user(virtual_device.data, bufSourceData, bufCount);
	durationL = 0;
	durationR = 0;
	return ret;
}

/****************************************************************************/
/* This function closes the device.                                         */
/****************************************************************************/
int device_close(struct inode *inode, struct file *filp){
	up(&virtual_device.sem);
	printk(KERN_INFO "evarobotEncoder: closed device");
	return 0;
}

struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = device_open,
	.release = device_close,
	.write = device_write,
	.read = device_read
};
 
/****************************************************************************/
/* Module init / cleanup block.                                             */
/****************************************************************************/
int r_init(void) {
 
   printk(KERN_NOTICE "evarobotEncoder: Hello !\n");
   r_int_config();
   encoder_init();
   
   ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
	
	if(ret < 0){
		printk(KERN_ALERT "evarobotEncoder: failed to allocate a major number");
		return ret;
	}

	major_number = MAJOR(dev_num);
	
	printk(KERN_INFO "evarobotEncoder: major number is %d", major_number);
	printk(KERN_INFO "\tuse \"mknod /dev/%s c %d 0\" for device file", DEVICE_NAME, major_number);

	mcdev = cdev_alloc();
	mcdev->ops = &fops;
	mcdev->owner = THIS_MODULE;

	ret = cdev_add(mcdev, dev_num, 1);
	
	if(ret < 0){
		printk(KERN_ALERT "evarobotEncoder: unable to add cdev to kernel");
		return ret;
	}

	sema_init(&virtual_device.sem, 1);

	
	return 0;
}
 
void r_cleanup(void) {
   printk(KERN_NOTICE "evarobotEncoder: Goodbye\n");
   r_int_release();
   
   	cdev_del(mcdev);

	unregister_chrdev_region(dev_num, 1);
	printk(KERN_ALERT "evarobotEncoder: unloaded module");
 
   return;
}
 
 
module_init(r_init);
module_exit(r_cleanup);
 
 
/****************************************************************************/
/* Module licensing/description block.                                      */
/****************************************************************************/
MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);

