#!/bin/sh

. /usr/share/debconf/confmodule

TARGET="/cdrom"
TMPFILE="/tmp/md5tmp"

cleanup() {
	[ -f "$TMPFILE" ] && rm -f $TMPFILE
}

# ask the user if we should check
db_set cdrom-checker/start "false"
db_fset cdrom-checker/start seen false
db_input high cdrom-checker/start
db_go
db_get cdrom-checker/start
[ "$RET" != "true" ] && exit 0

# check the cdrom, and exit on error
if [ ! -d "${TARGET}/.disk" -o ! -f "${TARGET}/md5sum.txt" ]; then
	db_fset cdrom-checker/wrongcd seen false
	db_input high cdrom-checker/wrongcd
	db_go
	exit 1
fi

cd $TARGET

# calculate the max percent
set -- `wc -l md5sum.txt`
mcount="$1"

# check each file seperate, we can use a process bar for this
#db_progress_start 0 $mcount cdrom-checker/progress_title

cleanup
count=0
cat md5sum.txt 2>/dev/null | \
while read LINE; do
	count=$(($count+1))
	set -- $LINE
	file=`echo $2 | sed -e 's/^\.\///'`
	# TODO:
	#db_subst cdrom-checker/progress_step FILE "$file"
	#db_progress_step 1 cdrom-checker/progress_step
	echo "$1  $2" >$TMPFILE
	md5sum -c $TMPFILE 1>/dev/null 2>&1
	if [ $? -ne 0 ]; then
		echo -n "$2" >$TMPFILE
		break
	fi
	cleanup
done

# stop the progress bar
#db_progress_stop

# some file is faulty, stop here
if [ -f "$TMPFILE" ]; then
	file=`cat $TMPFILE | sed -e 's/^\.\///'`
	db_subst cdrom-checker/missmatch FILE "$file"
	db_fset cdrom-checker/missmatch seen false
	db_input critical cdrom-checker/missmatch
	db_go
	cleanup
	echo "failed"
	exit 1
fi

# notice the user about success
db_fset cdrom-checker/passed seen false
db_input high cdrom-checker/passed
db_go
exit 0

