#!/bin/bash
#
# usage: l10n-spellcheck.sh
#
# l10n-spellcheck.sh - this script triggers all the spellchecking process
#
# Author: Davide Viti <zinosat@tiscali.it> 2005, for the Debian Project
#

PATH=~/aspell:~/gnuplot/bin:$PATH
export PATH

LOCAL_REPOSITORY=~/debian-installer
OUT_DIR=~/public_html/spellcheck/
NEW="check_$(date '+%Y%m%d')"

# update local copy of svn repository
svn up $LOCAL_REPOSITORY

# spellcheck and move data to public_html
sh check_all.sh $LOCAL_REPOSITORY
mv $NEW $OUT_DIR

# save symlink destinations
PREVIOUS=`ls -l $OUT_DIR/previous | sed "s:.*-> ::"`
LATEST=`ls -l $OUT_DIR/latest | sed "s:.*-> ::"`
SAVED_STATS=`echo ${NEW}.txt | sed "s:check_:stats_:"`

rm $OUT_DIR/previous
rm -r $OUT_DIR/$PREVIOUS

rm $OUT_DIR/latest

ln -s $OUT_DIR/$NEW    $OUT_DIR/latest
ln -s $OUT_DIR/$LATEST $OUT_DIR/previous

cp $OUT_DIR/$NEW/stats.txt $OUT_DIR/history/$SAVED_STATS
mv $OUT_DIR/$NEW/index.html $OUT_DIR

echo ""
echo "***  $SAVED_STATS  ***"

sh diff_stats.sh $OUT_DIR/$LATEST/stats.txt $OUT_DIR/$NEW/stats.txt
