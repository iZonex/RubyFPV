sudo mkdir -p /usr/local/bin/
sudo mkdir -p /etc/ruby/config/models/
sudo mkdir -p /var/lib/ruby/updates/
sudo mkdir -p /var/run/
sudo mkdir -p /media/usb/

sudo chown -R your_user:your_group /usr/local/bin/ruby/
sudo chown -R your_user:your_group /etc/ruby/
sudo chown -R your_user:your_group /var/lib/ruby/
sudo chown -R your_user:your_group /var/run/
sudo chown -R your_user:your_group /media/usb/

sudo chmod -R 755 /usr/local/bin/ruby/
sudo chmod -R 755 /etc/ruby/
sudo chmod -R 755 /var/lib/ruby/
sudo chmod -R 755 /var/run/
sudo chmod -R 755 /media/usb/