#!/bin/sh
#
# Author: Petter Reinholdtsen <pere@hungry.com>
# Date:   2002-12-08
#
# Update the web pages from CVS every 5 minutes.  Remove locally
# modified files, to enforce that every changes is done using CVS.

webdir=$HOME/public_html/debian-installer
mailto=pere@debian.org

tmpfile=/tmp/update-web-$$.log
difflog=/tmp/update-web-$$-diff.log
updatelog=/tmp/update-web-$$-update.log

mail=/usr/bin/mail
uuencode=/usr/bin/uuencode

PATH=$PATH:/usr/bin

exec > $tmpfile 2>&1 < /dev/null

# Make sure everyone can read the resulting files.
umask 022

cd $webdir || exit 1

cvs -q diff -u > $difflog < /dev/null

if test -s $difflog; then
    localmods=`grep ^Index: < $difflog |cut -d: -f2`
    echo rm -f $localmods

    echo
    echo "Removed locally modified files!"
    echo
    cat $difflog
    echo
fi

cvs -q update -d < /dev/null | egrep -v '^(P|U)' | tee $updatelog

badfiles=`grep '^?' $updatelog | cut -c2-`

for f in $badfiles; do
    echo
    echo "Removing  $f as it is not registered in CVS.  Current content:"
    $uuencode $f $f
    echo
    rm -f $f
done

# Make sure all the pages are readable, and all directories visible
chmod -R a+rX $webdir

if test -s $tmpfile; then
    cat $tmpfile | $mail -s "web-update from CVS (cron)" $mailto

    echo
    hostname=`hostname`
    echo "Report from $0 on $hostname"
fi

rm -f $difflog $tmpfile
