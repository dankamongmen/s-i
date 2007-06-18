lang en_GB
keyboard uk
auth --enablemd5 --enableshadow
bootloader --location=mbr
interactive
lilo
mouse --device=ttyS0 --emulthree msintellips/2
network --bootproto=static --ip=10.0.2.15 --netmask=255.255.255.0 --gateway=10.0.2.254 --nameserver=10.0.2.1
