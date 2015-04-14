
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include <unistd.h>		/* exit */
#include <sys/ioctl.h>		/* ioctl */

#define DEVICE "/dev/evarobotBumper"

#define MAX_BUMPER 3

#ifndef DEBUG
#define DEBUG
#endif

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


int main()
{
	int i, fd;
	char ch, write_buf[100], read_buf[100];

	i = 1;

	fd = open(DEVICE, O_RDWR);
	
	struct bumper_ioc_transfer pins;
	struct bumper_data data;
	
	int i_i;
	
	pins.i_gpio_pin[0] = 21;
	pins.i_gpio_pin[1] = 26;
	pins.i_gpio_pin[2] = 6;
	
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
			ioctl(fd, IOCTL_SET_BUMPERS, &pins);
			break;
		case 'a':
			
			for(i_i = 0; i_i < 3; i_i++)
			{
				data.i_bumper_no = i_i;
				ioctl(fd, IOCTL_READ_BUMPERS, &data);
				
				printf("Bumper[%d]:  %d \n", i_i, data.i_value);
			}
			
			break;
		case '0':
			data.i_bumper_no = 0;
			while(ioctl(fd, IOCTL_READ_BUMPERS, &data) != 1);
	//		ioctl(fd, IOCTL_READ_BUMPERS, &data);
			printf("Bumper[%d]:  %d \n", 0, data.i_value);
		break;
		case '1':
			data.i_bumper_no = 1;
			while(ioctl(fd, IOCTL_READ_BUMPERS, &data) != 1);
	//		ioctl(fd, IOCTL_READ_BUMPERS, &data);
			printf("Bumper[%d]:  %d \n", 1, data.i_value);
		break;
		case '2':
			data.i_bumper_no = 2;
			while(ioctl(fd, IOCTL_READ_BUMPERS, &data) != 1);
	//		ioctl(fd, IOCTL_READ_BUMPERS, &data);
			printf("Bumper[%d]:  %d \n", 2, data.i_value);
		break;

		
		default: 
			i = 0;
			break;

	}

	close(fd);
	return 0;

}
