#!/bin/bash
#
# usage: check_all
#
# check_all - run check_dit.sh on all the languages in lang2dict.txt
#
# Author: Davide Viti <zinosat@tiscali.it> 2004, for the Debian Project
#

DEST_DIR="check_$(date '+%Y%m%d')"
STATS=$DEST_DIR/stats
GNUPLOT_SCRIPT=$DEST_DIR/graph.gp
GNUPLOT_DATA=$DEST_DIR/data.txt


i=0
for LANGUAGE in `cat lang2dict.txt | sed "s:\(^#.*\)::"`; do
    LANG=`echo  ${LANGUAGE} | awk -F, '{print $1}'`
    DICT=`echo  ${LANGUAGE} | awk -F, '{print $2}'`

    echo ""
    echo "*** checking \"$LANG\" using aspell-${DICT} ***"
    ./check_dit.sh $LANG $DICT /home/zino/debian-installer/packages/ $DEST_DIR

    i=`expr $i + 1`
done

# Compute some statistics
i=0
TOTAL=0
AVERAGE=0
for VAL in `cat $STATS.txt | sort | awk '{print $1}'`; do
    TOTAL=`expr $TOTAL + $VAL`
    i=`expr $i + 1`
done
AVERAGE=`expr $TOTAL / $i`

# create HTML table
echo "<table cellpadding=\"5\" cellspacing=\"1\" border=\"2\">" > $STATS.html
cat $STATS.txt |\
sed "s:\(^[0-9]*\)\( [a-z_a-zA-Z ]*$\):<tr><td> <a href=\"check/\2.tar.gz\">\2</a></td> <td>\1</td></tr>:"|\
sed "s:/ :/:" >> $STATS.html
echo "<tr><td><b>Total<b></td> <td><b>$TOTAL<b></td></tr>" >> $STATS.html
echo "<tr><td><b>Average<b></td> <td><b>$AVERAGE<b></td></tr>" >> $STATS.html
echo "</table>" >> $STATS.html

# create plot using gnuplot
i=0
for ROW in `cat $STATS.txt | sort | awk '{print $2}'`; do
    XTICS=$(echo "$XTICS \"$ROW\" $i,")
    i=`expr $i + 1`
done

XTICS=`echo $XTICS | sed "s:^":\(":" | sed "s:,$:):"`

cat $STATS.txt | sort | awk '{print $1}' > $GNUPLOT_DATA

rm -f $GNUPLOT_SCRIPT
echo "set terminal png size 640,480" >> $GNUPLOT_SCRIPT
echo "set output \"$DEST_DIR/graph.png\"" >> $GNUPLOT_SCRIPT
echo "set title \"Statistics for the $i languages with an Aspell dictionary\"" >> $GNUPLOT_SCRIPT
echo "set key left" >> $GNUPLOT_SCRIPT
echo "set xlabel \"Languages\"" >> $GNUPLOT_SCRIPT
echo "set ylabel \"Unknown words\"" >> $GNUPLOT_SCRIPT
echo "set xtics $XTICS" >> $GNUPLOT_SCRIPT
echo "set xrange [-1:$i]" >> $GNUPLOT_SCRIPT
echo "plot \"$GNUPLOT_DATA\" with impulses,$AVERAGE t \"Average: $AVERAGE words\"" >> $GNUPLOT_SCRIPT

gnuplot $GNUPLOT_SCRIPT

rm -f $GNUPLOT_DATA
