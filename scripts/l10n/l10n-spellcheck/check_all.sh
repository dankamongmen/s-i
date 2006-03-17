#!/bin/bash
#
# -*-sh-*-
#
#     Copyright (C) 2004-2006 Davide Viti <zinosat@tiscali.it>
#
#     
#     This program is free software; you can redistribute it and/or
#     modify it under the terms of the GNU General Public License
#     as published by the Free Software Foundation; either version 2
#     of the License, or (at your option) any later version.
#     
#     This program is distributed in the hope that it will be useful,
#     but WITHOUT ANY WARRANTY; without even the implied warranty of
#     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#     GNU General Public License for more details.
#     
#     You should have received a copy of the GNU General Public License
#     along with this program; if not, write to the Free Software
#     Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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

    
    check_dit.sh ${LANG} ${DICT} ${DI_COPY} ${DEST_DIR}

    i=`expr $i + 1`
done

sort ${DEST_DIR}/all_codes-tmp.txt | uniq -c > ${DEST_DIR}/all_codes.txt

# Add a third column with the encoded character
#  567 - <U0033> "3"
sed "s|\(.* \)\(<U....>\)|\1\2  \"\2\" |" ${DEST_DIR}/all_codes.txt | uxx2utf > ${DEST_DIR}/all_codes-tmp.txt
mv ${DEST_DIR}/all_codes-tmp.txt ${DEST_DIR}/all_codes.txt

HERE=$(pwd)
cd ${DEST_DIR}/nozip
tar cf codepoints.tar *_codes.txt
mv codepoints.tar ${DEST_DIR} && cd ${DEST_DIR}
tar rf codepoints.tar all_codes.txt
gzip - < codepoints.tar > ${DEST_DIR}/zip/codepoints.tar.gz
rm codepoints.tar
cd ${HERE}

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
echo "set title \"$(date '+%Y%m%d') - ${PLOT_TITLE}\""
echo "set key left"
echo "set xlabel \"Languages [$i]\" 0,-1"
echo "set ylabel \"Unknown words\""
echo "set origin 0,0.01"
echo "set xtics rotate ${XTICS}"
echo "set xrange [-1:$i]"
echo "plot \"${GNUPLOT_DATA}\" t \"Unknown words count\" with impulses,${AVERAGE} t \"Average: ${AVERAGE} words\""

exec 1>&6 6>&-      # Restore stdout and close file descriptor #6.

gnuplot ${GNUPLOT_SCRIPT}

rm -f ${GNUPLOT_DATA}
rm -f ${GNUPLOT_SCRIPT}

