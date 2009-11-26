#!/bin/bash -e
#
# l10n support for win32-loader
# Copyright (C) 2007,2009  Robert Millan <rmh@aybabtu.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Got this from our parent
export LANGUAGE

# We need this for gettext to work.  Why not matching $LANGUAGE as passed
# by our parent?  Because then we'd have to guess country, and en_US.UTF-8
# just works.
export LANG=en_US.UTF-8

# LC_ALL is usually undefined in user shells.  Hence it's easy to experience
# the illusion that this line is unnecessary.  pbuilder thinks otherwise (as
# it exports LC_ALL=C breaking it).
export LC_ALL=$LANG

. /usr/bin/gettext.sh
export TEXTDOMAIN=win32-loader
export TEXTDOMAINDIR=${PWD}/locale

nsis_lang=`gettext LANG_ENGLISH`

langstring ()
{
  local string
  read string
  echo "LangString $1 \${$nsis_lang} \"$string\""
}

# translate:
# This must be the string used by GNU iconv to represent the charset used
# by Windows for your language.  If you don't know, check
# [wine]/tools/wmc/lang.c, or http://www.microsoft.com/globaldev/reference/WinCP.mspx
#
# IMPORTANT: In the rest of this file, only the subset of UTF-8 that can be
# converted to this charset should be used.
charset=`gettext windows-1252`

# translate:
# Charset used by NTLDR in your localised version of Windows XP.  If you
# don't know, maybe http://en.wikipedia.org/wiki/Code_page helps.
ntldr_charset=`gettext cp437`

# Were we asked to translate a single string?
if [ "$1" != "" ] ; then
  exec gettext -s "$1"
fi

# May be requested by our parent makefile (see above)
# translate:
# The name of your language _in English_ (must be restricted to ascii)
gettext English > /dev/null

# The bulk of the strings
./win32-loader | iconv -f utf-8 -t "${charset}"

# Now comes a string that may be used by NTLDR (or not).  So we need both
# samples.

#  - First we get the string.

# translate:
# IMPORTANT: only the subset of UTF-8 that can be converted to NTLDR charset
# (e.g. cp437) should be used in this string.  If you don't know which charset
# applies, limit yourself to ascii.
d_i=`gettext "Continue with install process"`

#  - Then we get a sample for bootmgr in the native charset.
echo "${d_i}" | iconv -f utf-8 -t "${charset}" | langstring d-i

#  - And another for ntldr in its own charset.  If the charset cannot be
#    converted to ${ntldr_charset}, fallback to English untill it's fixed.
(if echo "${d_i}" | iconv -f utf-8 -t "${ntldr_charset}" > /dev/null 2>&1 ; then
  echo "${d_i}" | iconv -f utf-8 -t "${ntldr_charset}"
else
  echo "Continue with install process"
fi) | langstring d-i_ntldr
