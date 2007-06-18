lang en_GB
langsupport --default=en_GB de_DE
keyboard uk
auth --enablemd5 --enableshadow
bootloader --location=mbr
interactive
lilo
%include tests/included.ks
mouse --device=ttyS0 --emulthree msintellips/2
network --bootproto=static --ip=10.0.2.15 --netmask=255.255.255.0 --gateway=10.0.2.254 --nameserver=10.0.2.1
part / --size=1024 --grow --maxsize=2048 --asprimary --fstype xfs
partition swap --size=512 --maxsize=1024
rootpw rootme
timezone --utc America/New_York
url --url http://archive.ubuntu.com/ubuntu

%packages
openssh-server
-man-db

%pre
#! /bin/sh

echo "This is a %pre script."
echo "It does nothing very interesting."
%post
#! /bin/sh

echo "This is a %post script."
echo "It does nothing very interesting, in a chroot."
%post --nochroot
#! /bin/sh

echo "This is a %post script."
echo "It does nothing very interesting, outside a chroot."
