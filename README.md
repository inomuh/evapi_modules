# evapi_modules
Linux kernel versiyonunuzu öğrenin

	> uname -a

Aşağıdaki linkten uygun kernel vesiyonunu indirin

	> rpi-update

Kaynak kodu çekmek için rpi-source komutu indirilir ve kullanılır.
	
	> sudo wget https://raw.githubusercontent.com/notro/rpi-source/master/rpi-source -O /usr/bin/rpi-source && sudo chmod +x /usr/bin/rpi-source && /usr/bin/rpi-source -q --tag-update
	> rpi-source --skip-gcc

Oluşturulacak driver'ların konulacağı klasör oluşturulur.

	> cd /lib/modules/$(uname -r)/kernel/drivers/
	> sudo mkdir evarobot

Driver'lar derlenir ve kopyalanır.

	> cd ~/evapi_modules/module_sonar
	> make
	> sudo cp driver_sonar.ko /lib/modules/$(uname -r)/kernel/drivers/evarobot/

	> cd ~/evapi_modules/module_encoder
	> make
	> sudo cp driver_encoder.ko /lib/modules/$(uname -r)/kernel/drivers/evarobot/

	> cd /lib/modules/$(uname -r)/kernel/drivers/evarobot/
	> sudo insmod driver_encoder.ko
	> sudo insmod driver_sonar.ko

Açılışta yüklenmesi otomatik için düzenlenecek dosyalar;

	> sudo nano /etc/rc.local
  		sudo bash /home/pi/evapi_modules/module_sonar/./setup_sonar_driver.sh 
  		sudo bash /home/pi/evapi_modules/module_encoder/./setup_encoder_driver.sh

	> sudo nano /etc/modules
  		snd-bcm2836
  		i2c-dev
  		i2c-bcm2709
  		driver_encoder
  		driver_sonar
