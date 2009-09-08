#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""Make a list of words that enchant spell checker does not
approve. Words are parsed from the files given as arguments.
One or more word list files can be given, those words are deemed
OK even if enchant would flag them wrong.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Library General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

Author: Tapio Lehtonen <tale@debian.org> 
"""

def handleCommandLine():
    """ Get arguments and options from command line. """
    from optparse import OptionParser

    usage = """usage: %prog [options] filename
Print out unknown words found in filename. Options
-p and --wordlist can be given several times."""
    lhelpstr = """What language is the text to proofread? For example fi_FI."""
    parser = OptionParser(usage=usage)
    parser.set_defaults(verbose=True)
    parser.add_option("-p", "--wordlist", 
                      action="append", type="string", dest="wordlists",
                      help="File with OK words one per line.",
                      metavar="Wordlist")
    parser.add_option("-d", "--language", 
                      action="store", type="string", dest="language",
                      help=lhelpstr)
    parser.add_option("-v", action="store_true", dest="verbose",
                      help="Be verbose.")
    parser.add_option("-q", action="store_false", dest="verbose",
                      help="Be quiet.")

    (options, args) = parser.parse_args()
    if len(args) != 1:
        parser.error("incorrect number of arguments")
    if options.verbose:
        if (options.wordlists == [] or options.wordlists == None):
            print "No extra wordlist files."
        else:
            print "Extra wordlists: ",
            for f in options.wordlists:
                print f + " ",
            print
        if options.language:
            print "Proofreading using language ", options.language, "."
        else:
            print "Proofreading using default language."
    
    return (options, args)


if __name__ == "__main__":
    
    from enchant.checker import SpellChecker
    import fi

    o, a = handleCommandLine()
    if o.verbose:
        print o
        print a

    try:
        textF = open(a[0], "r")
    except IOError, value:
        print "Erroria", value
        sys.exit(4)

    text = textF.readlines()
    textF.close()
    if o.verbose:
        print "Text as read:"
        print text 
        print "Text to check is:"
        for l in text:
            print l.strip()
    if o.verbose:
        print "Language is", o.language
    chkr = SpellChecker(o.language)
    if o.verbose:
        if chkr.wants_unicode:
            print "Checker wants Unicode text to check."
        else:
            print "Checker wants normal strings text to check."

    utext = u""
    for t in text:
        utext += unicode(t, "utf-8")
    chkr.set_text(utext)
    for err in chkr:
        print err.word
    

    
