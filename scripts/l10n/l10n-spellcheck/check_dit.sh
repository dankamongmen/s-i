#!/bin/bash
#
# usage: check_dit <lang> <aspell-suffix> <D-I repository path> <output dir>
#
# check_dit - d-i translations spell-checker
#
# Packages: aspell, aspell-bin, aspell-it, aspell-fr, etc (I tested it with it)
#
# Author: Davide Viti <zinosat@tiscali.it> 2004, for the Debian Project
#

usage() {
    echo  "Usage:"
    echo  "$0 <lang> <aspell-suffix> <D-I repository path> <output dir>"
}

initialise() {
GATHER_STRINGS_SCRIPT=./msgstr_extract.awk

ALL_STRINGS=$DEST_DIR/${LANG}_all.txt
NO_VARS=$DEST_DIR/1_no_vars_${LANG}
ALL_UNKNOWN=$DEST_DIR/2_all_unkn_${LANG}
UNKN=$DEST_DIR/${LANG}_unkn_wl.txt
}

checks(){

if [ ! -d $DI_COPY ] ; then
    echo $DI_COPY does not exist
    exit 1
fi

if [ ! -f $GATHER_STRINGS_SCRIPT ] ; then
    echo "$GATHER_STRINGS_SCRIPT does not exist. You need it!"
    exit 1
fi

dpkg -l aspell | grep -q "^ii"
if [ $? != 0 ] ; then
    echo "aspell is not installed"
    echo "you need some packages (aspell, aspell-bin, aspell-${LANG})"
    exit 1
fi

dpkg -l aspell-${DICT} | grep -q "^ii" 
if  [ $? != 0 ] ; then
    echo "There was an error during the detection of aspell-${DICT}"
    exit 1
fi


if [ ! -d $DEST_DIR ] ; then
    mkdir $DEST_DIR
fi

}

if [ -z "$4" ]
    then
    usage
    exit 1
fi

LANG=$1
DICT=$2
DI_COPY=$3
DEST_DIR=$4

initialise	# initalise some variables
checks		# do an environment check


# Build word list: it's composed by words common to all d-i translations
# AND words specific for each language:
#
# di_${LANG}_wl = di_common_wl + ${LANG}_wl
#
# FIXME: I assume that wlS have to be searched ./wls 
COMMON_WL=./wls/di_common_wl.txt
SPECIFIC_WL=./wls/${LANG}_wl.txt
WLIST=./wls/${LANG}_di_wl

WL_WARN=0
if [ ! -f $COMMON_WL ] ; then
    echo "can't find di_common_wl.txt. You'll get a lot of unknown technical words!"
    COMMON_WL=
    WL_WARN=`expr $WL_WARN + 1`
fi

if [ ! -f $SPECIFIC_WL ] ; then
    echo "can't find ${LANG}_wl.txt. You'll get a lot of unknown words in \"${LANG}\" language!"
    SPECIFIC_WL=
    WL_WARN=`expr $WL_WARN + 1`    
fi

# build wl only if there's at least one of the "private" wls
if [ $WL_WARN -ne 2 ] ; then
    cat $COMMON_WL $SPECIFIC_WL | sort -f > $WLIST.txt

# FIXME: does not work for accented letters despite --encoding=utf-8
# NB: --lang uses $LANG and not $DICT
    aspell --lang=$LANG create master ./$WLIST < $WLIST.txt

    WL_PARAM="--add-extra-dicts ./$WLIST"
fi

rm -f $ALL_STRINGS
for LANG_FILE in `find $DI_COPY -name "$LANG.po"`; do
    ENC=`cat $LANG_FILE | grep -e "^\"Content-Type:" | sed 's:^.*charset=::' | sed 's:\\\n\"::'`
    awk -f $GATHER_STRINGS_SCRIPT $LANG_FILE | iconv --from $ENC --to utf-8 >> $ALL_STRINGS
done

# Remove ${HOME} from the "*** path_of_po_file" string
cat $ALL_STRINGS | sed "s:\(.*\)${HOME}\(.*\):\1\2:" > $DEST_DIR/no_home.txt
mv $DEST_DIR/no_home.txt $ALL_STRINGS

# remove ${ALL_THESE_VARIABLES} which do not need to be spell checked
grep -e "^-" $ALL_STRINGS | sed s/\$\{[a-zA-Z0-9_]*\}//g > $NO_VARS

# spell check the selected strings eventually using a custom wl and utf-8 encoding (works?)
cat $NO_VARS | aspell -l --lang=$LANG --encoding=utf-8 $WL_PARAM > $ALL_UNKNOWN

# sort all the unrecognized words (don't care about uppe/lower case)
# count duplicates
# take note of unknown words
cat $ALL_UNKNOWN | sort -f | uniq -c > $UNKN

echo `wc -l $UNKN | awk '{print $1}'` $LANG >> ${DEST_DIR}/stats.txt

rm $NO_VARS $ALL_UNKNOWN

tar czf $DEST_DIR/${LANG}.tar.gz $ALL_STRINGS $UNKN
rm $ALL_STRINGS $UNKN

