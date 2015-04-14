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
   
   int i_int_no;
   int i; 
   
   // disable hard interrupts (remember them in flag 'flags')
   local_irq_save(flags);
   
      
   i_int_no = -1;
      
   
   for(i = 0; i < g_i_size_of_sonars; i++)
   {	   
	   if(irq == irq_pins[i])
	   {
		   i_int_no = i;
		   break;
	   }
	    
   }
   
   #ifdef DEBUG
		printk(KERN_NOTICE "evarobotSonar: Interrupt No: %d\n", i_int_no);
	#endif
   
	if(i_int_no >= 0)
	{
	   pin_val[i_int_no] = gpio_get_value(g_i_gpio_reflection[i_int_no]);
	   
	   if(pre_pin_val[i_int_no] == 0 && pin_val[i_int_no] == 1)
	   {
			// Saymaya Başla
			//clock_gettime(CLOCK_MONOTONIC, &tstart);
			getrawmonotonic(&tstart[i_int_no]);
	   }
	   else if(pre_pin_val[i_int_no] == 1 && pin_val[i_int_no] == 0)
	   {
			// Saymayı Durdur.
			//clock_gettime(CLOCK_MONOTONIC, &tstop);
			getrawmonotonic(&tstop[i_int_no]);

			flight_time = tstop[i_int_no].tv_nsec - tstart[i_int_no].tv_nsec;
			
			#ifdef DEBUG
				printk(KERN_NOTICE "evarobotSonar: flight_time: %ld\n", flight_time);
			#endif	
			
			g_i_distance[i_int_no] = (int)(172 * flight_time / 100000  - 309);

			if(flight_time > 40000000)
			{
				g_i_distance[i_int_no] = 50010;
			}

			if(g_i_distance[i_int_no] < 0)
			{
				g_i_distance[i_int_no] = 0;
			}

		g_i_new_data[i_int_no] = 1;
		
		#ifdef DEBUG
			printk(KERN_NOTICE "evarobotSonar: Distance[%d](10^4) %d\n", i_int_no, g_i_distance[i_int_no]);
		#endif
	   }

	pre_pin_val[i_int_no] = pin_val[i_int_no];
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

	getrawmonotonic(&g_ts);

	return;
}

 
 
/****************************************************************************/
/* This function releases interrupts.                                       */
/****************************************************************************/
void r_int_release(void) 
{
	int i;
	for(i = 0; i < g_i_size_of_sonars; i++)
	{
		free_irq(irq_pins[i], g_c_gpio_device_desc[i]);
		gpio_free(g_i_gpio_reflection[i]);
		
		gpio_unexport(g_i_gpio_trigger[i]);
		//gpio_unexport(g_i_gpio_reflection[i]);
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
	
//	sprintf( item_type.data, "%li_%li", durationL, durationR);
//	ret = copy_to_user(bufStoreData, item_type.data, bufCount);

	return ret;
}


/****************************************************************************/
/* This function writes to the device.                                      */
/****************************************************************************/
ssize_t device_write(struct file* filp, const char* bufSourceData, size_t bufCount, loff_t* curOffset)
{
	#ifdef DEBUG
		printk(KERN_INFO "evarobotSonar: writing to device");
	#endif
	//ret = copy_from_user(item_type.data, bufSourceData, bufCount);
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
	g_i_size_of_sonars = 0;
	
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
	struct sonar_data * dist_data;
	int i;
	char c_gpio_desc[15];
	//int m = 0;
	long ret;
	int i_free;	
	ret = 0;
	
	switch(ioctl_num)
	{
		case IOCTL_SET_PARAM:
		{

			for(i_free = 0; i_free < g_i_size_of_sonars; i_free++)
			{
				free_irq(irq_pins[i_free], g_c_gpio_device_desc[i_free]);
				gpio_free(g_i_gpio_reflection[i_free]);
				
				gpio_unexport(g_i_gpio_trigger[i_free]);
				//gpio_unexport(g_i_gpio_reflection[i]);
			}
			
			#ifdef DEBUG
				printk(KERN_NOTICE "evarobotSonar: Set Param ioctl is called.\n");
			#endif
			params = (struct sonar_ioc_transfer *) ioctl_param;
			
			g_i_size_of_sonars = params->i_size;
			
			if(g_i_size_of_sonars > MAX_SONAR)
			{
				printk(KERN_ALERT "evarobotSonar: Overloaded sensor number. ");
				return -1;
			}
			
			
			for(i = 0; i < g_i_size_of_sonars; i++)
			{
				g_i_gpio_reflection[i] = params->i_gpio_reflection[i];
				g_i_gpio_trigger[i] = params->i_gpio_trigger[i];
				
				gpio_export(g_i_gpio_reflection[i], true);
				gpio_export(g_i_gpio_trigger[i], true);
				
		//		gpio_direction_input(g_i_gpio_reflection[i]);
		//		gpio_direction_output(g_i_gpio_trigger[i]);
												
				sprintf(c_gpio_desc, "GPIO_%d_int", g_i_gpio_reflection[i]);
				sprintf(g_c_gpio_device_desc[i], "sonar%d", i);
				
				if (gpio_request(g_i_gpio_reflection[i], c_gpio_desc)) 
				{
					printk("evarobotSonar GPIO request faiure: %s\n", c_gpio_desc);
					return -1;
				}
				
				if ( (irq_pins[i] = gpio_to_irq(g_i_gpio_reflection[i])) < 0 ) 
 			    {
					printk("evarobotSonar: GPIO to IRQ mapping faiure %s\n", c_gpio_desc);
					return -1;
				}
				
				#ifdef DEBUG
					printk(KERN_NOTICE "evarobotSonar: Mapped int %d\n", irq_pins[i]);
				#endif
				
				if (request_irq(irq_pins[i],
						   (irq_handler_t ) r_irq_handler,
						   IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
				   //IRQF_TRIGGER_FALLING,
						   c_gpio_desc,
						   g_c_gpio_device_desc[i]) ) 
			   {
				  printk("evarobotSonar: Irq Request failure\n");
				  return -1;
			   }
				
			}
			
			//return g_i_size_of_sonars;
			break;
		}
		
		case IOCTL_READ_RANGE:
		{
			#ifdef DEBUG
				printk(KERN_NOTICE "evarobotSonar: Read Range ioctl is called.\n");
			#endif
			
			dist_data = (struct sonar_data *) ioctl_param;
			
			if(dist_data->i_sonar_no >= g_i_size_of_sonars)
			{
				printk(KERN_ALERT "evarobotSonar: Overloaded sensor number. ");
			}
			
			#ifdef DEBUG
				printk(KERN_NOTICE "evarobotSonar: Sonar NO: %d\n", dist_data->i_sonar_no);
			#endif
			
			//gpio_set_value(PIN24, 1L);
			//for(m=0; m<10; m++)
			//while(1)
		//	{
				gpio_direction_output(g_i_gpio_trigger[dist_data->i_sonar_no], 1);
				sleep_until(&g_ts, g_u_i_delay);
				gpio_direction_output(g_i_gpio_trigger[dist_data->i_sonar_no], 0);
				//gpio_set_value(g_i_gpio_trigger[dist_data->i_sonar_no], 0);
				sleep_until(&g_ts, g_u_i_delay);
				
		//	}
			//while(g_i_new_data[dist_data->i_sonar_no] < 1)
			
			#ifdef DEBUG
				printk(KERN_NOTICE "evarobotSonar: NEW Data: %d\n", g_i_new_data[dist_data->i_sonar_no]);
			#endif
			
			ret = g_i_new_data[dist_data->i_sonar_no];
			

			dist_data->i_distance = g_i_distance[dist_data->i_sonar_no];
			g_i_new_data[dist_data->i_sonar_no] = 0;
			
			break;
		}
		default:
			break;
	}
	
	return ret;
}
