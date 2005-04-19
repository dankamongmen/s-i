# Original Author: Frans Pop <aragorn@tiscali.nl>
# Modified by Davide Viti <zinosat@tiscali.it> in order to extract msgid entries
# 
# This AWK-script will extract all "msgid" strings from a .po file.

# It is supposed to be run with:
# find . -name it.po | xargs awk -f ~/bin/msgid_extract > [outputfile]

BEGIN {
    IN_MSGSTR=0;
}

{
    if (FILENAME != file) {
        IN_MSGSTR=0;
        file=FILENAME;
        print "";
	home_len=length(ENVIRON["HOME"])+2;
        print "*** " substr(file, home_len);
    }
    
    line=$0;
    if (IN_MSGSTR == 1) {
        if (NF > 0) {
	    if (substr(line, 1, 1) != "#" &&
	        index(line, "msgstr") == 0 &&
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
                index(line, "X-Poedit-Language:") == 0 &&
                index(line, "X-Poedit-Country:") == 0 &&
                index(line, "Plural-Forms:") == 0 &&
                index(line, "Content-Transfer-Encoding:") == 0 ) {
	        print "- " line
	    }
	} else {
	    IN_MSGSTR = 0
	}
    } else {
        if (NF > 0 && 
	    substr(line, 1, 1) != "#" && 
	    index(line, "msgid") > 0 ) {
	    IN_MSGSTR = 1;
	    line=substr(line, 7, length(line) - 6)
	    if (length(line) != 2)  {
	        print "- " line;
	    }
	}
    }
}
