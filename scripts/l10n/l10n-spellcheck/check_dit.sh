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

usage() {
    echo  "Usage:"
    echo  "$0 <lang> <aspell-suffix> <D-I repository path> <output dir>"
}

initialise() {
GATHER_MSGSTR_SCRIPT=msgstr_extract.awk
GATHER_MSGID_SCRIPT=msgid_extract.awk
CHECK_VAR=check_var.pl

ALL_STRINGS=${DEST_DIR}/${LANG}_all.txt
FILES_TO_KEEP="${ALL_STRINGS} ${FILES_TO_KEEP}"

NO_VARS=${DEST_DIR}/1_no_vars_${LANG}

ALL_UNKNOWN=${DEST_DIR}/2_all_unkn_${LANG}
NEEDS_RM="${ALL_UNKNOWN} ${NEEDS_RM}"

UNKN=${DEST_DIR}/${LANG}_unkn_wl.txt
FILES_TO_KEEP="${UNKN} ${FILES_TO_KEEP}"

SUSPECT_VARS=${DEST_DIR}/${LANG}_var.txt
}

checks(){

if [ ! -d ${BASE_SEARCH_DIR} ] ; then
    echo ${BASE_SEARCH_DIR} does not exist
    exit 1
fi

if [ ! -d ${DEST_DIR} ] ; then
    mkdir ${DEST_DIR}
fi

}

if [ -z "$4" ] ; then
    usage
    exit 1
fi

LANG=$1
DICT=$2
BASE_SEARCH_DIR=$3
DEST_DIR=$4

initialise	# initalise some variables
checks		# do an environment check

PO_FILE_LIST="${DEST_DIR}/${LANG}_file_list.txt"
NEEDS_RM="${PO_FILE_LIST} ${NEEDS_RM}"

sh ${PO_FINDER} ${BASE_SEARCH_DIR} ${LANG} > ${PO_FILE_LIST}

rm -f ${ALL_STRINGS}

for LANG_FILE in `cat ${PO_FILE_LIST}`; do
    ENC=`cat ${LANG_FILE} | grep -e "^\"Content-Type:" | sed 's:^.*charset=::' | sed 's:\\\n\"::'`

    echo "${LANG_FILE}" | grep -e ".po$" > /dev/null
    if  [ $? = 0 ] ; then

	echo ${ENC} | grep -iw "utf-8" > /dev/null
	if [ $? = 0 ]  ; then
	    if [ ${HANDLE_SUSPECT_VARS} = "yes" ] ; then
		${CHECK_VAR} -s ${LANG_FILE} >> ${SUSPECT_VARS}
	    fi
	    ${GATHER_MSGSTR_SCRIPT} ${LANG_FILE} >> ${ALL_STRINGS}
	else
	    if [ ${HANDLE_SUSPECT_VARS} = "yes" ] ; then
		${CHECK_VAR} -s ${LANG_FILE} | iconv --from ${ENC} --to utf-8 >> ${SUSPECT_VARS}
	    fi
	    ${GATHER_MSGSTR_SCRIPT} ${LANG_FILE} | iconv --from ${ENC} --to utf-8 >> ${ALL_STRINGS}
	fi

    else			# now deal with ".pot" files
	if [ ${HANDLE_SUSPECT_VARS} = "yes" ] ; then
	    ${CHECK_VAR} -s ${LANG_FILE} >> ${SUSPECT_VARS}
	fi
	${GATHER_MSGID_SCRIPT} ${LANG_FILE} >> ${ALL_STRINGS}
    fi
done

if [ ${HANDLE_SUSPECT_VARS} = "yes" ] ; then
    if [ `ls -l ${SUSPECT_VARS} | awk '{print $5}'` -gt 0 ]; then
	FILES_TO_KEEP="${SUSPECT_VARS} ${FILES_TO_KEEP}" 
	SUSPECT_EXIST=1
    else
	rm ${SUSPECT_VARS}
	SUSPECT_EXIST=0
    fi
fi

# remove ${ALL_THESE_VARIABLES} if they do not need to be spell checked
if [ ${REMOVE_VARS} = "yes" ] ; then
    NEEDS_RM="${NO_VARS} ${NEEDS_RM}"
    grep -e "^-" ${ALL_STRINGS} | sed s/\$\{[a-zA-Z0-9_]*\}//g > ${NO_VARS}
    FILE_TO_CHECK=${NO_VARS}
else
    FILE_TO_CHECK=${ALL_STRINGS}
fi

# if a binary wl exists, use it
if [ -f ${WLS_PATH}/${LANG}_di_wl ] ; then
    WL_PARAM="--add-extra-dicts ${WLS_PATH}/${LANG}_di_wl"
fi

if [ ${DICT} != "null" ] ; then
# spell check the selected strings eventually using a custom wl 
    cat ${FILE_TO_CHECK} | aspell list --lang=${LANG} --encoding=utf-8 ${WL_PARAM} ${ASPELL_EXTRA_PARAM} > ${ALL_UNKNOWN}

# sort all the unrecognized words (don't care about upper/lower case)
# count duplicates
# take note of unknown words
    cat ${ALL_UNKNOWN} | sort -f | uniq -c > ${UNKN}

# if we're *not* handling suspecet vars (i.e. d-i manual), make this an empty string
    if [ ${HANDLE_SUSPECT_VARS} = "no" ] ; then
	SUSPECT_EXIST=
    fi

# build the entry of stats.txt for the current language (i.e "395 it 1")
    echo `wc -l ${UNKN} | awk '{print $1}'` ${LANG} ${SUSPECT_EXIST} >> ${DEST_DIR}/stats.txt
else
    if [ ${HANDLE_SUSPECT_VARS} = "no" ] ; then
	SUSPECT_EXIST=
    fi

    echo "-1 ${LANG} ${SUSPECT_EXIST}" >> ${DEST_DIR}/stats.txt

    NEEDS_RM=`echo ${NEEDS_RM} | sed "s:${ALL_UNKNOWN}::"`
    FILES_TO_KEEP=`echo ${FILES_TO_KEEP} | sed "s:${UNKN}::"`
fi

rm ${NEEDS_RM}

if [ ! -d  ${DEST_DIR}/zip ] ; then
    mkdir ${DEST_DIR}/zip
fi

if [ ! -d  ${DEST_DIR}/nozip ] ; then
    mkdir ${DEST_DIR}/nozip
fi

# in the tgz file WL is iso-8859-1 (the way it has to be)
# in "nozip" it's utf-8 like the other files 
if [ -f ${WLS_PATH}/${LANG}_wl.txt ] ; then
    WORDLIST=${WLS_PATH}/${LANG}_wl.txt
    cat ${WORDLIST} | iconv --from iso-8859-1 --to utf-8 > ${DEST_DIR}/nozip/${LANG}_wl.txt
    cp ${WORDLIST} ${DEST_DIR}
    WL_ISO8859=${DEST_DIR}/${LANG}_wl.txt
fi

# now let's copy the common wl *only* to nozip
if [ -f ${WLS_PATH}/di_common_wl.txt ] ; then
    cp ${WLS_PATH}/di_common_wl.txt ${DEST_DIR}/nozip
fi

pushd ${DEST_DIR} > /dev/null

FILES_TO_KEEP=`echo ${FILES_TO_KEEP} | sed "s|${DEST_DIR}\/||g"`
WL_ISO8859=`echo ${WL_ISO8859} | sed "s|${DEST_DIR}\/||g"`
tar czf zip/${LANG}.tar.gz ${FILES_TO_KEEP} ${WL_ISO8859}

mv ${FILES_TO_KEEP} ${DEST_DIR}/nozip
rm -f ${WL_ISO8859}

popd > /dev/null

echo "AddCharset UTF-8 .txt" > ${DEST_DIR}/nozip/.htaccess
