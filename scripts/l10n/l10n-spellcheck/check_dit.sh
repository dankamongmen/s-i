#!/bin/bash
#
# usage: check_dit <lang> <aspell-suffix> <D-I repository path> <output dir>
#
# check_dit - d-i translations spell-checker
#
# Packages: aspell, aspell-bin, aspell-${DICT}
#
# Author: Davide Viti <zinosat@tiscali.it> 2004, for the Debian Project
#

usage() {
    echo  "Usage:"
    echo  "$0 <lang> <aspell-suffix> <D-I repository path> <output dir>"
}

initialise() {
GATHER_MSGSTR_SCRIPT=./msgstr_extract.awk
GATHER_MSGID_SCRIPT=./msgid_extract.awk
CHECK_VAR=./check_var.pl

ALL_STRINGS=$DEST_DIR/${LANG}_all.txt
FILES_TO_KEEP="$ALL_STRINGS $FILES_TO_KEEP"

NO_VARS=$DEST_DIR/1_no_vars_${LANG}
NEEDS_RM="$NO_VARS $NEEDS_RM"

ALL_UNKNOWN=$DEST_DIR/2_all_unkn_${LANG}
NEEDS_RM="$ALL_UNKNOWN $NEEDS_RM"

UNKN=$DEST_DIR/${LANG}_unkn_wl.txt
FILES_TO_KEEP="$UNKN $FILES_TO_KEEP"

SUSPECT_VARS=$DEST_DIR/${LANG}_var.txt
}

checks(){

if [ ! -d $DI_COPY ] ; then
    echo $DI_COPY does not exist
    exit 1
fi

if [ ! -f $GATHER_MSGSTR_SCRIPT ] ; then
    echo "$GATHER_MSGSTR_SCRIPT does not exist. You need it!"
    exit 1
fi

if [ ! -f $GATHER_MSGID_SCRIPT ] ; then
    echo "$GATHER_MSGID_SCRIPT does not exist. You need it!"
    exit 1
fi

if [ ! -f $CHECK_VAR ] ; then
    echo "$CHECK_VAR does not exist. You need it!"
    exit 1
fi

# This has to be commented in order to run this script on a system (like Alioth) 
# where we have a custom installation for aspell, or on a non-debian system

# dpkg -l aspell | grep -q "^ii"
# if [ $? != 0 ] ; then
#     echo "aspell is not installed"
#     echo "you need some packages (aspell, aspell-bin, aspell-${LANG})"
#     exit 1
# fi

# dpkg -l aspell-${DICT} | grep -q "^ii" 
# if  [ $? != 0 ] ; then
#     echo "There was an error during the detection of aspell-${DICT}"
#     exit 1
# fi


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
# "| perl -00lne'/\n/&&print'" would remove blank lines from wl: is it needed?
if [ $WL_WARN -ne 2 ] ; then
    cat $COMMON_WL $SPECIFIC_WL | sort -f | sed "s:\(^#.*\)::" > $WLIST.txt

# FIXME: does not work for accented letters despite --encoding=utf-8
# NB: --lang uses $LANG and not $DICT
    aspell --lang=$LANG create master ./$WLIST < $WLIST.txt

    WL_PARAM="--add-extra-dicts ./$WLIST"
fi

PO_FILE_LIST="${LANG}_file_list.txt"
NEEDS_RM="$PO_FILE_LIST $NEEDS_RM"

# Create a list of all the po files and count them
find $DI_COPY -name "$LANG.po" > $PO_FILE_LIST
NUM_PO_FILES=`wc -l $PO_FILE_LIST | awk '{print $1}'`

# Deal with "master files" so that we don't count twice unknown words:
#
# 1) there are both po files and master file (i.e. "fr")
#      ignore the master file
# 2) there's only the master file file (i.e. "ga") 
#      leave it unchanged
# 3) there's just a bunch of sparse po files and no master file (i.e. "it")
#      leave them unchanged   
# 4) English language if made of "en.po" + "templates.pot"

if [ $NUM_PO_FILES -gt 1 ]; then
    PO_FILE_LIST_NO_MASTER="${LANG}_no_master.txt"
    NEEDS_RM="$PO_FILE_LIST_NO_MASTER $NEEDS_RM"
    cat $PO_FILE_LIST | sed "s:$DI_COPY/po/$LANG.po::" > $PO_FILE_LIST_NO_MASTER
    PO_FILE_LIST=$PO_FILE_LIST_NO_MASTER
fi

rm -f $ALL_STRINGS
for LANG_FILE in `cat $PO_FILE_LIST`; do
    ENC=`cat $LANG_FILE | grep -e "^\"Content-Type:" | sed 's:^.*charset=::' | sed 's:\\\n\"::'`
    $CHECK_VAR -s $LANG_FILE | iconv --from $ENC --to utf-8 >> $SUSPECT_VARS
    awk -f $GATHER_MSGSTR_SCRIPT $LANG_FILE | iconv --from $ENC --to utf-8 >> $ALL_STRINGS
done

if [ $LANG = en ] ; then
    find $DI_COPY -name "templates.pot" >> $PO_FILE_LIST

    for LANG_FILE in `cat $PO_FILE_LIST | grep "templates.pot$"`; do
	$CHECK_VAR -s $LANG_FILE >> $SUSPECT_VARS
	awk -f $GATHER_MSGID_SCRIPT $LANG_FILE >> $ALL_STRINGS
    done
fi

if [ `ls -l $SUSPECT_VARS | awk '{print $5}'` -gt 0 ]; then
    FILES_TO_KEEP="$SUSPECT_VARS $FILES_TO_KEEP" 
    SUSPECT_EXIST=1
else
    rm $SUSPECT_VARS
    SUSPECT_EXIST=0
fi

# Remove ${HOME} from the "*** path_of_po_file" string
cat $ALL_STRINGS | sed "s:\(.*\)${HOME}\(.*\):\1\2:" > $DEST_DIR/no_home.txt
mv $DEST_DIR/no_home.txt $ALL_STRINGS

# remove ${ALL_THESE_VARIABLES} which do not need to be spell checked
grep -e "^-" $ALL_STRINGS | sed s/\$\{[a-zA-Z0-9_]*\}//g > $NO_VARS

# spell check the selected strings eventually using a custom wl and utf-8 encoding (works?)
cat $NO_VARS | aspell list --lang=$LANG --encoding=utf-8 $WL_PARAM > $ALL_UNKNOWN

# sort all the unrecognized words (don't care about upper/lower case)
# count duplicates
# take note of unknown words
cat $ALL_UNKNOWN | sort -f | uniq -c > $UNKN

# build the entry of stats.txt for the current language (i.e "395 it 1")
echo `wc -l $UNKN | awk '{print $1}'` $LANG $SUSPECT_EXIST >> ${DEST_DIR}/stats.txt

rm $NEEDS_RM

if [ ! -d  $DEST_DIR/zip ] ; then
    mkdir $DEST_DIR/zip
fi

if [ ! -d  $DEST_DIR/nozip ] ; then
    mkdir $DEST_DIR/nozip
fi

tar czf $DEST_DIR/${LANG}.tar.gz $FILES_TO_KEEP

mv $DEST_DIR/${LANG}.tar.gz $DEST_DIR/zip
mv $FILES_TO_KEEP $DEST_DIR/nozip

echo "AddCharset UTF-8 .txt" > $DEST_DIR/nozip/.htaccess
