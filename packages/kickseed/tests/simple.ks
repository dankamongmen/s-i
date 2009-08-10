lang en_GB
langsupport --default=en_US de_DE xh_ZA
keyboard uk
autostep
auth --enablemd5 --enableshadow --enablenis
bootloader --location=mbr
device eth module1 --opts="aic152x=0x340 io=11"
device scsi module2 --opts="testopts=testvalue"
firewall --disabled
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
repo --name test --baseurl http://mirror/ubuntu --distribution local --components=main,restricted --source --key=http://mirror/ubuntu/project/key
repo --name "another test" --baseurl http://mirror/other --key=http://mirror/ubuntu/project/key2
user cjwatson --fullname="Colin Watson" --password="foobar"
xconfig --resolution 1280x1024
preseed test/question1 string hello
preseed --owner base-config test/question2 boolean true
preseed apt-setup/security_host string ""

%packages
@ Ubuntu Desktop
openssh-server
-man-db

%pre
#! /bin/sh

echo "This is a %pre script."
echo "It does nothing very interesting."
%post --nochroot --interpreter /bin/dash
#! /bin/sh

echo "This is a %post script."
echo "It does nothing very interesting, outside a chroot."
echo "A warning should be printed for the attempt to use dash."
%post --interpreter=/bin/bash
#! /bin/bash

echo "This is a %post script."
echo "It does nothing very interesting, in a chroot."
