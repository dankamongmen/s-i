# Original Author: Frans Pop <aragorn@tiscali.nl>
# Modified by Davide Viti <zinosat@tiscali.it> in order to check italian translations
# 
# This AWK-script will extract all translated strings from a .po file.

# It is supposed to be run with:
# find . -name it.po | xargs awk -f ~/bin/msgstr_extract > [outputfile]

BEGIN {
    IN_MSGSTR=0;
}

{
    if (FILENAME != file) {
        IN_MSGSTR=0;
        file=FILENAME;
        print "";
        print "*** " file;
    }
    
    line=$0;
    if (IN_MSGSTR == 1) {
        if (NF > 0) {
	    if (substr(line, 1, 1) != "#" &&
	        index(line, "Project-Id-Version:") == 0 &&
                index(line, "Report-Msgid-Bugs-To:") == 0 &&
                index(line, "POT-Creation-Date:") == 0 &&
                index(line, "PO-Revision-Date:") == 0 &&
                index(line, "Last-Translator:") == 0 &&
                index(line, "Language-Team:") == 0 &&
                index(line, "org>") == 0 &&
                index(line, "MIME-Version:") == 0 &&
                index(line, "Content-Type:") == 0 &&
                index(line, "X-Generator:") == 0 &&
                index(line, "Content-Transfer-Encoding:") == 0 ) {
	        print "- " line
	    }
	} else {
	    IN_MSGSTR = 0
	}
    } else {
        if (NF > 0 && substr(line, 1, 1) != "#" && index(line, "msgstr") > 0) {
	    IN_MSGSTR = 1;
	    line=substr(line, 8, length(line) - 7)
	    if (length(line) != 2) {
	        print "- " line;
	    }
	}
    }
}
