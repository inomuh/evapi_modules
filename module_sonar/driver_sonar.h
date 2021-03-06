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

// Zaman icin
#include <linux/time.h>
#include <linux/delay.h>

#define DRIVER_AUTHOR "Mehmet Akcakoca <mehmet.akcakoca@inovasyonmuhendislik.com>"
#define DRIVER_DESC   "Evarobot Sonar Driver"
#define DEVICE_NAME   "evarobotSonar"

unsigned int g_u_i_delay = 50*1000;
unsigned int g_u_i_hb_delay = 150*1000;
 
//#ifndef DEBUG
//#define DEBUG
//#endif

#define MAX_SONAR 7

struct sonar_ioc_transfer 
{
	int i_gpio_pin;
	int i_sonar_id;
};

struct sonar_data
{
	int i_sonar_id;
	int i_distance;
};

#define SNRDRIVER_IOCTL_BASE 0xAE

		
#define IOCTL_SET_PARAM _IOW(SNRDRIVER_IOCTL_BASE, 0, struct sonar_ioc_transfer) /* Set Param */
#define IOCTL_READ_RANGE _IOWR(SNRDRIVER_IOCTL_BASE, 1, struct sonar_data) /* Read range */


struct fake_device
{
	char data[100];
	struct semaphore sem;
}item_type;


int g_i_gpio_pins[MAX_SONAR];

long g_i_distance[MAX_SONAR];
char g_c_gpio_device_desc[MAX_SONAR][8];

int g_i_sonar_no = 0;
int g_i_hb_is_ok[MAX_SONAR];
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
struct timespec tstart[MAX_SONAR];
struct timespec tstop[MAX_SONAR];
struct timespec timeout;
struct timespec g_ts;

int pin_val[MAX_SONAR];
int pre_pin_val[MAX_SONAR];

 
/****************************************************************************/
/* Interrupts variables block                                               */
/****************************************************************************/
short int irq_pins[MAX_SONAR]  = {0};



// Prototypes
static irqreturn_t r_irq_handler(int irq, void *dev_id, struct pt_regs *regs);
void sonar_init(void);
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



