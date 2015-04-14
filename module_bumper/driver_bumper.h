#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
 
#include <linux/interrupt.h>
#include <linux/gpio.h>

#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <asm/uaccess.h>

#include <linux/ioctl.h>

//#ifndef DEBUG
//#define DEBUG
//#endif

#define DRIVER_AUTHOR "Mehmet Akcakoca <mehmet.akcakoca@inovasyonmuhendislik.com>"
#define DRIVER_DESC   "Evarobot Bumper Driver"
#define DEVICE_NAME   "evarobotBumper"


#define MAX_BUMPER 3

struct bumper_ioc_transfer 
{
	int i_gpio_pin[MAX_BUMPER];
	int i_size; 

};

struct bumper_data
{
	int i_bumper_no;
	int i_value;
};

#define BUMPER_IOCTL_BASE 0xB0

		
#define IOCTL_SET_BUMPERS _IOW(BUMPER_IOCTL_BASE, 0, struct bumper_ioc_transfer) /* Set Param */
#define IOCTL_READ_BUMPERS _IOWR(BUMPER_IOCTL_BASE, 1, struct bumper_data) /* Read range */


struct fake_device
{
	char data[100];
	struct semaphore sem;
}item_type;


int g_i_size_of_bumpers = 0;
int g_i_gpio_pins[MAX_BUMPER];

char g_c_gpio_device_desc[MAX_BUMPER][9];

int g_i_new_data[MAX_BUMPER];

/****************************************************************************/
/* Driver variables                                                         */
/****************************************************************************/
struct cdev *mcdev;
int major_number;
int ret;

dev_t dev_num;

/****************************************************************************/
/* Sonar variables                                                        */
/****************************************************************************/
int pin_val[MAX_BUMPER] = {1, 1, 1};

 
/****************************************************************************/
/* Interrupts variables block                                               */
/****************************************************************************/
short int irq_pins[MAX_BUMPER]  = {0};



// Prototypes
static irqreturn_t r_irq_handler(int irq, void *dev_id, struct pt_regs *regs);
void r_int_release(void);
int device_open(struct inode *inode, struct file *filp);
ssize_t device_read(struct file* filp, char* bufStoreData, size_t bufCount, loff_t* curOffset);
ssize_t device_write(struct file* filp, const char* bufSourceData, size_t bufCount, loff_t* curOffset);
int device_close(struct inode *inode, struct file *filp);

long device_ioctl(struct file * filp, unsigned int ioctl_num, unsigned long ioctl_param);


static struct file_operations fops = {
        .owner = THIS_MODULE,
        .open = device_open,
        .release = device_close,
        .write = device_write,
        .read = device_read,
        .unlocked_ioctl = device_ioctl,
};


int r_init(void);
void r_cleanup(void);
module_init(r_init);
module_exit(r_cleanup);
 
 
/****************************************************************************/
/* Module licensing/description block.                                      */
/****************************************************************************/
MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);



