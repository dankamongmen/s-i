#!/bin/sh
set -e

if [ "$1" = "-r" ]; then
	lastok=$(egrep '^Exporting revision.*done' log|tail -n 1 | awk '{print $3}')
	if [ -n "$lastok" ]; then
		next=$(expr $lastok + 1)
		inc="--resume-from $next"
		tee="-a"
		echo "resuming from revision $next"
	else
		echo "resume failure"
	fi
fi

if [ ! "$inc" ]; then
	rm -rf git
fi

for p in $(grep 'create repository ' d-i.conf | sed 's/create repository //'); do 
	if [ ! -d git/$p ]; then
		mkdir  -p git/$p
		cd git/$p
		git init --bare
		cd ../..
	fi
done

(
	cd git
	../svn-all-fast-export/svn-all-fast-export \
		--identity-map=../authors-file $inc ../d-i.conf ../svn/d-i
) 2>&1 | tee $tee log

crashes=$(ls -1 git/fast_import_crash_* 2>/dev/null || true)
if [ -n "$crashes" ]; then
	echo "* Crashes detected: $crashes" >&2
	exit 1
fi
