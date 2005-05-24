#!/bin/bash
#
# usage: check_all <D-I repository path> <output dir>
#
# check_all - runs check_dit.sh on all the languages listed in lang2dict.txt
#
# Packages: gnuplot and the packages required by check_dit.sh
#
# Author: Davide Viti <zinosat@tiscali.it> 2004, for the Debian Project
#

usage() {
    echo  "Usage:"
    echo  "$0 <D-I repository path> <output dir>"
}

initialise(){
    STATS=${DEST_DIR}/stats
    GNUPLOT_SCRIPT=${DEST_DIR}/graph.gp
    GNUPLOT_DATA=${DEST_DIR}/data.txt
}

checks()
{
    if [ ! -d ${DI_COPY} ] ; then
	echo ${DI_COPY} does not exist
	exit 1
    fi
}

if [ -z "$1" ] ; then
    usage
    exit 1
fi

DI_COPY=$1
DEST_DIR=$2

initialise	# initalise some variables
checks          # do an environment check

i=0
for LANGUAGE in `cat ${LANGUAGE_LIST} | sed "s:\(^#.*\)::"`; do
    LANG=`echo  ${LANGUAGE} | awk -F, '{print $1}'`
    DICT=`echo  ${LANGUAGE} | awk -F, '{print $2}'`

    if [ ${#DICT} = 0 ] ; then
	DICT=null
	echo "*** checking \"$LANG\" dictionary not available ***"
    else
	echo "*** checking \"$LANG\" using aspell-${DICT} ***"
    fi

    
    ./check_dit.sh ${LANG} ${DICT} ${DI_COPY} ${DEST_DIR}

    i=`expr $i + 1`
done

if [ ! -d ${DEST_DIR} ] ; then
    echo does not exist: for some reasons nothing could be checked
    exit 1
fi

# create plot using gnuplot
i=0
TOTAL=0
AVERAGE=0

for ROW in `cat ${STATS}.txt | sort -n | sed 's: :,:g'`; do
    VAL=`echo ${ROW}| awk -F, '{print $1}'`
    LANG=`echo ${ROW}| awk -F, '{print $2}'`

    if [ ${VAL} -ne -1 ] ; then
	XTICS=$(echo "${XTICS} \"${LANG}\" $i,")
	TOTAL=`expr ${TOTAL} + ${VAL}`
	i=`expr $i + 1`
	echo ${VAL} >> ${GNUPLOT_DATA}
    fi
done

XTICS=`echo ${XTICS} | sed "s:^":\(":" | sed "s:,$:):"`

# avoid division by 0
if [ $i -ne 0 ] ; then
    AVERAGE=`expr ${TOTAL} / $i`
fi

LOGFILE=${GNUPLOT_SCRIPT}
exec 6>&1           # Link file descriptor #6 with stdout.
                    # Saves stdout.

exec > ${LOGFILE}     # stdout replaced with file "logfile.txt".

#echo "set terminal png size 640,480"
echo "set terminal png"
echo "set output \"${DEST_DIR}/graph.png\""
echo "set title \"${PLOT_TITLE} ($i languages)\""
echo "set key left"
echo "set xlabel \"Languages\" 0,-1"
echo "set ylabel \"Unknown words\""
echo "set origin 0,0.01"
echo "set xtics rotate ${XTICS}"
echo "set xrange [-1:$i]"
echo "plot \"${GNUPLOT_DATA}\" with impulses,${AVERAGE} t \"Average: ${AVERAGE} words\""

exec 1>&6 6>&-      # Restore stdout and close file descriptor #6.

gnuplot ${GNUPLOT_SCRIPT}

rm -f ${GNUPLOT_DATA}
rm -f ${GNUPLOT_SCRIPT}

