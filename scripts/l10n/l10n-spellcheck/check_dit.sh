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

ALL_UNKNOWN=$DEST_DIR/2_all_unkn_${LANG}
NEEDS_RM="$ALL_UNKNOWN $NEEDS_RM"

UNKN=$DEST_DIR/${LANG}_unkn_wl.txt
FILES_TO_KEEP="$UNKN $FILES_TO_KEEP"

SUSPECT_VARS=$DEST_DIR/${LANG}_var.txt
}

checks(){

if [ ! -d $BASE_SEARCH_DIR ] ; then
    echo $BASE_SEARCH_DIR does not exist
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
BASE_SEARCH_DIR=$3
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

# NB: --lang uses $LANG and not $DICT
    aspell --lang=$LANG create master ./$WLIST < $WLIST.txt

    WL_PARAM="--add-extra-dicts ./$WLIST"
fi

PO_FILE_LIST="${LANG}_file_list.txt"
NEEDS_RM="$PO_FILE_LIST $NEEDS_RM"

sh $PO_FINDER $BASE_SEARCH_DIR $LANG > $PO_FILE_LIST

rm -f $ALL_STRINGS
for LANG_FILE in `cat $PO_FILE_LIST`; do
    ENC=`cat $LANG_FILE | grep -e "^\"Content-Type:" | sed 's:^.*charset=::' | sed 's:\\\n\"::'`

    echo "$LANG_FILE" | grep -e ".po$" > /dev/null
    if  [ $? = 0 ] ; then

	echo $ENC | grep -iw "utf-8" > /dev/null
	if [ $? = 0 ]  ; then
	    if [ $HANDLE_SUSPECT_VARS = "yes" ] ; then
		$CHECK_VAR -s $LANG_FILE >> $SUSPECT_VARS
	    fi
	    awk -f $GATHER_MSGSTR_SCRIPT $LANG_FILE >> $ALL_STRINGS
	else
	    if [ $HANDLE_SUSPECT_VARS = "yes" ] ; then
		$CHECK_VAR -s $LANG_FILE | iconv --from $ENC --to utf-8 >> $SUSPECT_VARS
	    fi
	    awk -f $GATHER_MSGSTR_SCRIPT $LANG_FILE | iconv --from $ENC --to utf-8 >> $ALL_STRINGS
	fi

    else			# now deal with ".pot" files
	if [ $HANDLE_SUSPECT_VARS = "yes" ] ; then
	    $CHECK_VAR -s $LANG_FILE >> $SUSPECT_VARS
	fi
	awk -f $GATHER_MSGID_SCRIPT $LANG_FILE >> $ALL_STRINGS
    fi
done

if [ $HANDLE_SUSPECT_VARS = "yes" ] ; then
    if [ `ls -l $SUSPECT_VARS | awk '{print $5}'` -gt 0 ]; then
	FILES_TO_KEEP="$SUSPECT_VARS $FILES_TO_KEEP" 
	SUSPECT_EXIST=1
    else
	rm $SUSPECT_VARS
	SUSPECT_EXIST=0
    fi
fi

# remove ${ALL_THESE_VARIABLES} if they do not need to be spell checked
if [ $REMOVE_VARS = "yes" ] ; then
    NEEDS_RM="$NO_VARS $NEEDS_RM"
    grep -e "^-" $ALL_STRINGS | sed s/\$\{[a-zA-Z0-9_]*\}//g > $NO_VARS
    FILE_TO_CHECK=$NO_VARS
else
    FILE_TO_CHECK=$ALL_STRINGS
fi

# spell check the selected strings eventually using a custom wl 
cat $FILE_TO_CHECK | aspell list --lang=$LANG --encoding=utf-8 $WL_PARAM $ASPELL_EXTRA_PARAM > $ALL_UNKNOWN

# sort all the unrecognized words (don't care about upper/lower case)
# count duplicates
# take note of unknown words
cat $ALL_UNKNOWN | sort -f | uniq -c > $UNKN

# if we're *not* handling suspecet vars (i.e. d-i manual), make this an empty string
if [ $HANDLE_SUSPECT_VARS = "no" ] ; then
    SUSPECT_EXIST=
fi

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
