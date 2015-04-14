#include "driver_bumper.h"

 
/****************************************************************************/
/* IRQ handler - fired on interrupt                                         */
/****************************************************************************/
static irqreturn_t r_irq_handler(int irq, void *dev_id, struct pt_regs *regs) 
{
 
   unsigned long flags;
   
   int i_int_no;
   int i; 
   
   // disable hard interrupts (remember them in flag 'flags')
   local_irq_save(flags);
   
      
   i_int_no = -1;
      
   
   for(i = 0; i < g_i_size_of_bumpers; i++)
   {	   
	   if(irq == irq_pins[i])
	   {
		   i_int_no = i;
		   break;
	   }
	    
   }
   
   #ifdef DEBUG
		printk(KERN_NOTICE "evarobotBumper: Interrupt No: %d\n", i_int_no);
	#endif
   
	if(i_int_no >= 0)
	{
	   pin_val[i_int_no] = gpio_get_value(g_i_gpio_pins[i_int_no]);
	   g_i_new_data[i_int_no] = 1;
	 
	   #ifdef DEBUG
	   printk(KERN_NOTICE "evarobotBumper: Pin Val: %d\n", pin_val[i_int_no]);
	   #endif
	}
	else
	{
		printk(KERN_ALERT "evarobotBumper: Undefined interrupt\n");
	}

 

 
   // restore hard interrupts
   local_irq_restore(flags);
 
   return IRQ_HANDLED;
}
 
 
/****************************************************************************/
/* This function releases interrupts.                                       */
/****************************************************************************/
void r_int_release(void) 
{
	int i;
	for(i = 0; i < g_i_size_of_bumpers; i++)
	{
		free_irq(irq_pins[i], g_c_gpio_device_desc[i]);
		gpio_free(g_i_gpio_pins[i]);
		
		//gpio_unexport(g_i_gpio_trigger[i]);
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
		printk(KERN_ALERT "evarobotBumper: could not lock device during open");
		return -1;
	}

	#ifdef DEBUG
		printk(KERN_INFO "evarobotBumper: opened device");
	#endif
	
	return 0;
}

/****************************************************************************/
/* This function reads from the device.                                     */
/****************************************************************************/
ssize_t device_read(struct file* filp, char* bufStoreData, size_t bufCount, loff_t* curOffset)
{
	#ifdef DEBUG
		printk(KERN_INFO "evarobotBumper: reading from device");
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
		printk(KERN_INFO "evarobotBumper: writing to device");
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
	printk(KERN_INFO "evarobotBumper: closed device");
	return 0;
}


 
/****************************************************************************/
/* Module init / cleanup block.                                             */
/****************************************************************************/
int r_init(void)
{
	printk(KERN_NOTICE "evarobotBumper: Opening !\n");

   ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
	
	if(ret < 0)
	{
		printk(KERN_ALERT "evarobotBumper: failed to allocate a major number");
		return ret;
	}

	major_number = MAJOR(dev_num);
	
	#ifdef DEBUG
		printk(KERN_INFO "evarobotBumper: major number is %d", major_number);
		printk(KERN_INFO "\tuse \"mknod /dev/%s c %d 0\" for device file", DEVICE_NAME, major_number);
	#endif
	
	mcdev = cdev_alloc();
	mcdev->ops = &fops;
	mcdev->owner = THIS_MODULE;

	ret = cdev_add(mcdev, dev_num, 1);
	
	if(ret < 0)
	{
		printk(KERN_ALERT "evarobotBumper: unable to add cdev to kernel");
		return ret;
	}

	sema_init(&item_type.sem, 1);
	g_i_size_of_bumpers = 0;
	
	return 0;
}
 
void r_cleanup(void) 
{
	printk(KERN_NOTICE "evarobotBumper: Closing...\n");

   r_int_release();
   
   	cdev_del(mcdev);

	unregister_chrdev_region(dev_num, 1);
	printk(KERN_ALERT "evarobotBumper: unloaded module");
 
   return;
}


long device_ioctl(struct file * filp, unsigned int ioctl_num, unsigned long ioctl_param)
{
	struct bumper_ioc_transfer * params; 
	struct bumper_data * bumperData;
	int i;
	char c_gpio_desc[15];
	//int m = 0;
	long ret;
	int i_free;	
	ret = 0;
	
	switch(ioctl_num)
	{
		case IOCTL_SET_BUMPERS:
		{

			for(i_free = 0; i_free < g_i_size_of_bumpers; i_free++)
			{
				free_irq(irq_pins[i_free], g_c_gpio_device_desc[i_free]);
				gpio_free(g_i_gpio_pins[i_free]);
				
				//gpio_unexport(g_i_gpio_trigger[i_free]);
				//gpio_unexport(g_i_gpio_reflection[i]);
			}
			
			#ifdef DEBUG
				printk(KERN_NOTICE "evarobotBumper: Set Param ioctl is called.\n");
			#endif
			params = (struct bumper_ioc_transfer *) ioctl_param;
			
			g_i_size_of_bumpers = params->i_size;
			
			if(g_i_size_of_bumpers > MAX_BUMPER)
			{
				printk(KERN_ALERT "evarobotBumper: Overloaded sensor number. ");
				return -1;
			}
			
			
			for(i = 0; i < g_i_size_of_bumpers; i++)
			{
				g_i_gpio_pins[i] = params->i_gpio_pin[i];
				
				gpio_export(g_i_gpio_pins[i], true);
												
				sprintf(c_gpio_desc, "GPIO_%d_int", g_i_gpio_pins[i]);
				sprintf(g_c_gpio_device_desc[i], "bumper%d", i);
				
				if (gpio_request(g_i_gpio_pins[i], c_gpio_desc)) 
				{
					printk("evarobotBumper GPIO request faiure: %s\n", c_gpio_desc);
					return -1;
				}
				
				if ( (irq_pins[i] = gpio_to_irq(g_i_gpio_pins[i])) < 0 ) 
 			    {
					printk("evarobotSonar: GPIO to IRQ mapping faiure %s\n", c_gpio_desc);
					return -1;
				}
				
				#ifdef DEBUG
					printk(KERN_NOTICE "evarobotBumper: Mapped int %d\n", irq_pins[i]);
				#endif
				
				if (request_irq(irq_pins[i],
						   (irq_handler_t ) r_irq_handler,
						   IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
						   c_gpio_desc,
						   g_c_gpio_device_desc[i]) ) 
			   {
				  printk("evarobotBumper: Irq Request failure\n");
				  return -1;
			   }
				
			}
			
			//return g_i_size_of_sonars;
			break;
		}
		
		case IOCTL_READ_BUMPERS:
		{
			#ifdef DEBUG
				printk(KERN_NOTICE "evarobotBumper: Read Range ioctl is called.\n");
			#endif
			
			bumperData = (struct bumper_data *) ioctl_param;
			
			if(bumperData->i_bumper_no >= g_i_size_of_bumpers)
			{
				printk(KERN_ALERT "evarobotBumper: Overloaded sensor number. ");
			}
			
			#ifdef DEBUG
				printk(KERN_NOTICE "evarobotBumper: Bumper NO: %d\n", bumperData->i_bumper_no);
			#endif
			
			
			#ifdef DEBUG
				printk(KERN_NOTICE "evarobotBumper: NEW Data: %d\n", g_i_new_data[bumperData->i_bumper_no]);
			#endif
			
			ret = g_i_new_data[bumperData->i_bumper_no];
			

			bumperData->i_value = pin_val[bumperData->i_bumper_no];
			g_i_new_data[bumperData->i_bumper_no] = 0;
			
			break;
		}
		default:
			break;
	}
	
	return ret;
}
