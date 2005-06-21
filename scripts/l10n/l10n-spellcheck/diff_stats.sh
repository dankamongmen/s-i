#!/bin/bash
#
# -*-sh-*-
#
#     Copyright (C) 2004-2005 Davide Viti <zinosat@tiscali.it>
# 
#     This program is free software; you can redistribute it and/or modify
#     it under the terms of the GNU General Public License as published by
#     the Free Software Foundation; either version 2 of the License, or
#     (at your option) any later version.
# 
#     This program is distributed in the hope that it will be useful,
#     but WITHOUT ANY WARRANTY; without even the implied warranty of
#     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#     GNU General Public License for more details.
# 
#     You should have received a copy of the GNU General Public
#     License along with this program; if not, write to the Free
#     Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
#     MA 02111-1307 USA
#
#

usage() {
    echo  "Usage:"
    echo  "$0 <old stats> <new stats>"
}

OLD=$1
NEW=$2

if [ $# -ne 2 ]; then
    usage
    exit 1
fi

if [ ! -f $OLD ] ; then
    $OLD does not exist
    usage
    exit 1
fi

if [ ! -f $NEW ] ; then
    $NEW does not exist
    usage
    exit 1
fi

echo ""
echo "-------------------------"
printf "%-8s %4s %4s %4s\n" "LANG" "OLD" "NEW" "DELTA"
echo "-------------------------"

for ROW in `cat ${NEW} | sed 's: :,:g'`
  do

  UNKN=`echo  ${ROW} | awk -F, '{print $1}'`
  LANG=`echo  ${ROW} | awk -F, '{print $2}'`
  
  OLD_UNKN=`grep -w ${LANG} ${OLD}`

  if [ $? = 0 ]; then
      OLD_UNKN=`echo ${OLD_UNKN} | awk '{print $1}'`
      DELTA=`expr ${UNKN} - ${OLD_UNKN}`
      NEW_ENTRY=0
  else
      OLD_UNKN="N/A" # new language not present in <old stats>
      NEW_ENTRY=1
  fi


  if [ $NEW_ENTRY = 1 ]; then
      DELTA=${UNKN}
  fi
  
  if [ $DELTA = 0 ]; then
      DELTA="."
  fi
      
  printf "%-8s %4s %4s %4s\n" $LANG $OLD_UNKN $UNKN $DELTA

done
