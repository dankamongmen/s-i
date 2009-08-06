#!/bin/sh

TOP=`pwd`
for p in $(grep 'create repository ' d-i.conf | sed 's/create repository //'); do 
	cd $TOP/git/$p
	git gc --aggressive
	git update-server-info
done
