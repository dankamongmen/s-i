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

# associate a .rng file to each .ttf file
# the rng file contains a sorted list of the unicode coordinates contained in  the ttf file
for FONTFILE in $(find $1 -name *.ttf) ; do
    RANGEFILE="ranges/$(basename ${FONTFILE} | sed "s:.ttf$:.rng:")"
    ./showttf ${FONTFILE} | grep "^Glyph" | awk '{print $7}' | sort | uniq | sed "s|^U+||" > ${RANGEFILE}
done

rm -f all_ranges.txt
find ranges -name *.rng | sort > rangefiles.txt
for RANGEFILE in $(cat rangefiles.txt) ; do    
    cat ${RANGEFILE} >> all_ranges.txt
done

# create a list of the unicode coords used at least on one font file
sort all_ranges.txt | uniq > used_coords.txt

# For each unicode coordinate used in one or more ttf files produce this stats
#
# 0000 1 0 1 0 1
# 0020 0 0 0 1 1 

i=0
num_loops=5
ranges=$(cat rangefiles.txt)

exec 6>&1 
exec > tbl.txt
for COORD in $(cat used_coords.txt) ; do
    echo -n "${COORD}:"

    for RANGEFILE in $ranges ; do
	grep --mmap -F -q -w ${COORD} ${RANGEFILE}
	if [ $? = 0 ]; then
	    echo -n "1:"  # found
	else
	    echo -n "0:"  # not found
	fi
    done

    echo ""
##
## For profiling: 
##                time * num_lines(used_coords.txt) / num_loops
##

#     i=`expr $i + 1`
#     if [ $i -eq ${num_loops} ] ; then
#	 echo "</tabella>"
#	 exit 0
#     fi
done

exec 1>&6 6>&-

cat rangefiles.txt | sed "s|ranges/||" > tmp.1
mv tmp.1 rangefiles.txt
cat rangefiles.txt tbl.txt | sed "s|:$||" > map.txt
