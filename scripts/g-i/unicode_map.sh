#!/bin/bash
# -*-sh-*-
#
#     Copyright (C) 2005 Davide Viti <zinosat@tiscali.it>
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
# showttf was grabbed from:
#     http://cvs.sourceforge.net/viewcvs.py/fontforge/fontforge/fonttools/showttf.c
#
# to compile it:
#     gcc -o showttf showttf.c
#
# ./unicode_map.sh <ttf files path> 
#
# Ex: sh unicode_map.sh ~/g-i/installer/build/tmp/gtk-miniiso/tree/usr/share/fonts/truetype/
#
# produces various files (needs to be cleaned up)
# 
# map.txt shows which font files contain a particular glyph (the glyph listed are at least on one
# ttf file: i.e. it's not from U+0000 to U+ffff)
#
# the colums correspond (same order) to the font names listed in the header
#
# TODO: alot of things. it should be easy to use XML / HTML to create a nice looking table

if [ ! -f ./showttf ]
    then
    echo "Error: showttf is needed to run this script"
    exit 1
fi

if [ ! -d $1 ] ; then
    echo "$1 does not exist"
    exit 1
fi

rm -fr ranges
mkdir ranges

rm -f fontfiles.txt ttfname.txt
find $1 -name *.ttf > fontfiles.txt
for FONTFILE in `cat fontfiles.txt` ; do
    basename ${FONTFILE} >> ttfname.txt
done


# associate a .rng file to each .ttf file
# the rng file contains a sorted list of the unicode coordinates contained in  the ttf file
for FONTFILE in `cat fontfiles.txt` ; do
    RANGEFILE="ranges/$(basename ${FONTFILE} | sed "s:.ttf$:.rng1:")"
    ./showttf ${FONTFILE} | grep "^Glyph" | awk '{print $7}' | sort | uniq > ${RANGEFILE}
    cat $RANGEFILE | sed "s|U|$(basename ${FONTFILE}):U|" > "ranges/$(basename ${RANGEFILE} | sed "s:.rng1$:.rng:")"
    rm $RANGEFILE
done

rm -f all_ranges.txt
find ranges -name *.rng | sort > rangefiles.txt
for RANGEFILE in $(cat rangefiles.txt) ; do    
    cat ${RANGEFILE} >> all_ranges.txt
done

# create a list of the unicode coords used at least on one font file
cat all_ranges.txt | awk -F: '{print $2}' | sort | uniq > used_coords.txt

# For each unicode coordinate used in one or more ttf files produce this stats
#
# U+0000 1 0 1 0 1
# U+0020 0 0 0 1 1 
rm -f tbl.txt

i=0
num_loops=500

for COORD in `cat used_coords.txt` ; do
    echo -n "${COORD}:" >> tbl.txt

    for RANGEFILE in `cat rangefiles.txt` ; do
	grep -F -q -w ${COORD} ${RANGEFILE} 
	if [ $? = 0 ]; then
	    echo -n "1:"   >> tbl.txt  # found
	else
	    echo -n "0:"   >> tbl.txt  # not found
	fi
	
    done

    echo ""  >> tbl.txt

##
## For profiling: 
##                time * num_lines(used_coords.txt) / num_loops
##

#     i=`expr $i + 1`
#     if [ $i -eq ${num_loops} ] ; then
# 	exit 0
#     fi
done

cat rangefiles.txt | sed "s|ranges/||" > tmp.1
mv tmp.1 rangefiles.txt
cat rangefiles.txt tbl.txt | sed "s|:$||" > map.txt
