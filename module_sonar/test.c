
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include <unistd.h>		/* exit */
#include <sys/ioctl.h>		/* ioctl */

#define DEVICE "/dev/evarobotSonar"

#define MAX_SONAR 7

struct sonar_ioc_transfer 
{
	int i_gpio_reflection[MAX_SONAR];
	int	i_gpio_trigger[MAX_SONAR];
	int i_size;

};

struct sonar_data
{
	int i_sonar_no;
	int i_distance;
};

#define SNRDRIVER_IOCTL_BASE 0xAE

		
#define IOCTL_SET_PARAM _IOW(SNRDRIVER_IOCTL_BASE, 0, struct sonar_ioc_transfer) /* Set Param */
#define IOCTL_READ_RANGE _IOWR(SNRDRIVER_IOCTL_BASE, 1, struct sonar_data) /* Read range */


int main()
{
	int i, fd;
	char ch, write_buf[100], read_buf[100];

	i = 1;

	fd = open(DEVICE, O_RDWR);
	
	struct sonar_ioc_transfer pins;
	struct sonar_data data;
	
	int i_i;
	
	pins.i_gpio_reflection[0] = 23;
	pins.i_gpio_trigger[0] = 24;
	
	pins.i_gpio_reflection[1] = 25;
	pins.i_gpio_trigger[1] = 7;
	
	pins.i_gpio_reflection[2] = 16;
	pins.i_gpio_trigger[2] = 20;
	
	pins.i_size = 3;
	
	
	
	
	if(fd < 0){
		printf("file %s either does not exist or has been locked by another process\n", DEVICE);
		exit(-1);
	}

	printf("r=read\n w=write\nenter command: ");
	scanf("%c", &ch);
	
	
	
	switch (ch){
		case 'w':
			printf("enter data: ");
			scanf(" %[^\n]", write_buf);
			write(fd, write_buf, sizeof(write_buf));
			break;
		case 'r':
			read(fd, read_buf, sizeof(read_buf));
			printf("device: %s\n", read_buf);
			break;
		case 'i':
			ioctl(fd, IOCTL_SET_PARAM, &pins);
			break;
		case 'a':
			
			for(i_i = 0; i_i < 3; i_i++)
			{
				data.i_sonar_no = i_i;
				ioctl(fd, IOCTL_READ_RANGE, &data);
				
				printf("Sonar Distance[%d]:  %d \n", i_i, data.i_distance);
			}
			
			break;
		case '0':
			data.i_sonar_no = 0;
			while(ioctl(fd, IOCTL_READ_RANGE, &data) != 1);
			
			printf("Sonar Distance[%d]:  %d \n", 0, data.i_distance);
		break;
		case '1':
			data.i_sonar_no = 1;
			while(ioctl(fd, IOCTL_READ_RANGE, &data) != 1);
			
			printf("Sonar Distance[%d]:  %d \n", 1, data.i_distance);
		break;
		case '2':
			data.i_sonar_no = 2;
			while(ioctl(fd, IOCTL_READ_RANGE, &data) != 1);
			
			printf("Sonar Distance[%d]:  %d \n", 2, data.i_distance);
		break;

		
		default: 
			i = 0;
			break;

	}

	close(fd);
	return 0;

}
