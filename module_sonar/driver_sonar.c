#include "driver_sonar.h"


static void sleep_until(struct timespec *ts, int delay)
{
	
	ts->tv_nsec += delay;
	if(ts->tv_nsec >= 1000*1000*1000){
		ts->tv_nsec -= 1000*1000*1000;
		ts->tv_sec++;
	}
//	clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, ts, NULL);
	ndelay(delay);
}
 
/****************************************************************************/
/* IRQ handler - fired on interrupt                                         */
/****************************************************************************/
static irqreturn_t r_irq_handler(int irq, void *dev_id, struct pt_regs *regs) 
{
 
	unsigned long flags;
	long flight_time;

	// disable hard interrupts (remember them in flag 'flags')
	local_irq_save(flags);
	
	g_i_distance = -1;
		
	if(irq == irq_pins[g_i_sonar_no])
	{
		pin_val = gpio_get_value(g_i_gpio_pins[g_i_sonar_no]);
		#ifdef DEBUG
			printk(KERN_NOTICE "evarobotSonar: pin_val: %d\n", pin_val);
		#endif	

		if(pre_pin_val == 0 && pin_val == 1)
		{
			// Start counting.
			//clock_gettime(CLOCK_MONOTONIC, &tstart);
			getrawmonotonic(&tstart);
		}
		else if(pre_pin_val == 1 && pin_val == 0)
		{
			// Stop counting.
			//clock_gettime(CLOCK_MONOTONIC, &tstop);
			getrawmonotonic(&tstop);

			flight_time = tstop.tv_nsec - tstart.tv_nsec;

			#ifdef DEBUG
				printk(KERN_NOTICE "evarobotSonar: flight_time: %ld\n", flight_time);
			#endif	

			g_i_distance = (int)(172 * flight_time / 100000  - 309);

			if(flight_time > 40000000)
			{
				g_i_distance = 50010;
			}

			if(g_i_distance < 0)
			{
				g_i_distance = 0;
			}

			#ifdef DEBUG
				printk(KERN_NOTICE "evarobotSonar: Distance[%d](10^4) %d\n", g_i_sonar_no, g_i_distance);
			#endif
		}
		
		pre_pin_val = pin_val;
	}
	else
	{
		printk(KERN_ALERT "evarobotSonar: Undefined interrupt\n");
	}
 
	// restore hard interrupts
	local_irq_restore(flags);
	return IRQ_HANDLED;
}
 
 
/****************************************************************************/
/* This function inits sonar.                                             */
/****************************************************************************/
void sonar_init(void)
{
	int i;
	for(i = 0; i < MAX_SONAR; i++)
		g_i_gpio_pins[i] = -1;
		
	getrawmonotonic(&g_ts);
	return;
}

 
/****************************************************************************/
/* This function releases interrupts.                                       */
/****************************************************************************/
void r_int_release(void) 
{
	int i;
		
	for(i = 0; i < MAX_SONAR; i++)
	{
		if(g_i_gpio_pins[i] >= 0)
		{
			free_irq(irq_pins[i], g_c_gpio_device_desc[i]);
			gpio_free(g_i_gpio_pins[i]);
			
			gpio_unexport(g_i_gpio_pins[i]);
		}
	}
 
    
   return;
}
 
/****************************************************************************/
/* This function opens the device 				                            */
/****************************************************************************/ 
int device_open(struct inode *inode, struct file *filp)
{

	if(down_interruptible(&item_type.sem) != 0)
	{
		printk(KERN_ALERT "evarobotSonar: could not lock device during open");
		return -1;
	}

	#ifdef DEBUG
		printk(KERN_INFO "evarobotSonar: opened device");
	#endif
	
	return 0;
}

/****************************************************************************/
/* This function reads from the device.                                     */
/****************************************************************************/
ssize_t device_read(struct file* filp, char* bufStoreData, size_t bufCount, loff_t* curOffset)
{
	#ifdef DEBUG
		printk(KERN_INFO "evarobotSonar: reading from device");
	#endif
	
	sprintf( item_type.data, "%d", g_i_distance);
	ret = copy_to_user(bufStoreData, item_type.data, bufCount);

	return ret;
}


/****************************************************************************/
/* This function writes to the device.                                      */
/****************************************************************************/
ssize_t device_write(struct file* filp, const char* bufSourceData, size_t bufCount, loff_t* curOffset)
{
	unsigned long flags;
	long l_sonar_no;
	
	#ifdef DEBUG
		printk(KERN_INFO "evarobotSonar: writing to device");
	#endif
	
	local_irq_save(flags);
	
	ret = copy_from_user(item_type.data, bufSourceData, bufCount);
	
	ret = kstrtol(item_type.data, 10, &l_sonar_no);
	
	g_i_sonar_no = (int)l_sonar_no;
	
	g_i_distance = -1;
		
	if(g_i_sonar_no >= MAX_SONAR)
	{
		return -1;
	}
	
	gpio_direction_output(g_i_gpio_pins[g_i_sonar_no], 1);
	sleep_until(&g_ts, g_u_i_delay);
	gpio_direction_output(g_i_gpio_pins[g_i_sonar_no], 0);

	gpio_direction_input(g_i_gpio_pins[g_i_sonar_no]);
	
	// restore hard interrupts
	local_irq_restore(flags);
	return ret;
}

/****************************************************************************/
/* This function closes the device.                                         */
/****************************************************************************/
int device_close(struct inode *inode, struct file *filp)
{
	up(&item_type.sem);
	printk(KERN_INFO "evarobotSonar: closed device");
	return 0;
}


 
/****************************************************************************/
/* Module init / cleanup block.                                             */
/****************************************************************************/
int r_init(void)
{
	printk(KERN_NOTICE "evarobotSonar: Opening !\n");

   sonar_init();
   
   ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
	
	if(ret < 0)
	{
		printk(KERN_ALERT "evarobotSonar: failed to allocate a major number");
		return ret;
	}

	major_number = MAJOR(dev_num);
	
	#ifdef DEBUG
		printk(KERN_INFO "evarobotSonar: major number is %d", major_number);
		printk(KERN_INFO "\tuse \"mknod /dev/%s c %d 0\" for device file", DEVICE_NAME, major_number);
	#endif
	
	mcdev = cdev_alloc();
	mcdev->ops = &fops;
	mcdev->owner = THIS_MODULE;

	ret = cdev_add(mcdev, dev_num, 1);
	
	if(ret < 0)
	{
		printk(KERN_ALERT "evarobotSonar: unable to add cdev to kernel");
		return ret;
	}

	sema_init(&item_type.sem, 1);
	
	return 0;
}
 
void r_cleanup(void) 
{
	printk(KERN_NOTICE "evarobotSonar: Closing...\n");

   r_int_release();
   
   	cdev_del(mcdev);

	unregister_chrdev_region(dev_num, 1);
	printk(KERN_ALERT "evarobotSonar: unloaded module");
 
   return;
}


long device_ioctl(struct file * filp, unsigned int ioctl_num, unsigned long ioctl_param)
{
	struct sonar_ioc_transfer * params; 
	char c_gpio_desc[15];
	//int m = 0;
	long ret;

	ret = 0;
	
	switch(ioctl_num)
	{
		case IOCTL_SET_PARAM:
		{						
			#ifdef DEBUG
				printk(KERN_NOTICE "evarobotSonar: Set Param ioctl is called.\n");
			#endif
			
			params = (struct sonar_ioc_transfer *) ioctl_param;
								
			if(params->i_sonar_id >= MAX_SONAR)
			{
				printk(KERN_ALERT "evarobotSonar: Overloaded sensor number. ");
				return -1;
			}
			
			// Clear pre interrupt
			if(g_i_gpio_pins[params->i_sonar_id] >= 0)
			{
				free_irq(irq_pins[params->i_sonar_id], g_c_gpio_device_desc[params->i_sonar_id]);
				gpio_free(g_i_gpio_pins[params->i_sonar_id]);
			}	
						
			// Init interrupts
			g_i_gpio_pins[params->i_sonar_id] = params->i_gpio_pin;
			gpio_export(g_i_gpio_pins[params->i_sonar_id], true);
			
			sprintf(c_gpio_desc, "GPIO_%d_int", g_i_gpio_pins[params->i_sonar_id]);
			sprintf(g_c_gpio_device_desc[params->i_sonar_id], "sonar%d", params->i_sonar_id);
			
			if (gpio_request(g_i_gpio_pins[params->i_sonar_id], c_gpio_desc)) 
			{
				printk("evarobotSonar GPIO request faiure: %s\n", c_gpio_desc);
				return -1;
			}
			
			if ( (irq_pins[params->i_sonar_id] = gpio_to_irq(g_i_gpio_pins[params->i_sonar_id])) < 0 ) 
			{
				printk("evarobotSonar: GPIO to IRQ mapping faiure %s\n", c_gpio_desc);
				return -1;
			}
				
			#ifdef DEBUG
				printk(KERN_NOTICE "evarobotSonar: Mapped int %d\n", irq_pins[params->i_sonar_id]);
			#endif
			
			if (request_irq(irq_pins[params->i_sonar_id],
											(irq_handler_t ) r_irq_handler,
											IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
											//IRQF_TRIGGER_FALLING,
											c_gpio_desc,
											g_c_gpio_device_desc[params->i_sonar_id]) ) 
			 {
					printk("evarobotSonar: Irq Request failure\n");
					return -1;
			 }			
			
			break;
		}
		
		default:
			break;
	}

	return ret;
}
