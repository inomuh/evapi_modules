obj-m += driver_bumper.o

all:
	make -C /home/pi/linux M=$(PWD) modules
clean:
	rm -rf *.o *.ko *.mod.* *.symvers *.order *~

