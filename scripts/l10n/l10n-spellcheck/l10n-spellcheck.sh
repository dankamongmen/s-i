#!/bin/bash
PATH=~/aspell:~/gnuplot/bin:$PATH
export PATH

LOCAL_REPOSITORY=~/debian-installer
NEW="check_$(date '+%Y%m%d')"
OUT_DIR=~/public_html/spellcheck/

# update local copy of svn repository
cd $LOCAL_REPOSITORY
svn up

# spellcheck and move data to public_html
cd ~/l10n-spellcheck
sh check_all.sh $LOCAL_REPOSITORY
mv $NEW $OUT_DIR

cd  $OUT_DIR

# save symlink destinations
PREVIOUS=`ls -l previous | sed "s:.*-> ::"`
LATEST=`ls -l latest | sed "s:.*-> ::"`
SAVED_STATS=`echo ${NEW}.txt | sed "s:check_:stats_:"`

rm previous
rm -r $PREVIOUS
rm latest

ln -s $NEW latest
ln -s $LATEST previous

cp $NEW/stats.txt history/$SAVED_STATS

echo ""
echo "***  $SAVED_STATS  ***"
echo ""
cat history/$SAVED_STATS
