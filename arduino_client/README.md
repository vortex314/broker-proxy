# if flashing stellaris fails
https://docs.platformio.org/en/latest/faq.html#platformio-udev-rules
'''
curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core/master/scripts/99-platformio-udev.rules | sudo tee /etc/udev/rules.d/99-platformio-udev.rules

sudo service udev restart
sudo usermod -a -G dialout $USER
sudo usermod -a -G plugdev $USER
'''
reboot

docker run -p 6379:6379 -it --rm redislabs/redistimeseries 

sudo chown -R $USER /usr/local/lib/node_modules