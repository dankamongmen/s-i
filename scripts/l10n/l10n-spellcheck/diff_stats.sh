#!/bin/bash
#
# usage: diff_stats.sh <old stats> <new stats>
#
# diff_stats.sh - shows diff between stats files
#
# Author: Davide Viti <zinosat@tiscali.it> 2005, for the Debian Project
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

for ROW in `cat ${NEW} | sed 's: :,:'`
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
