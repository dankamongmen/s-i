#!/usr/bin/env python
# -*- coding: utf-8 -*-

import enchant

def handleCommandLine():
    """ Get arguments and options from command line. """
    from optparse import OptionParser

    usage = """usage: %prog [options] filename
Print out unknown words found in filename. Options
-p and --wordlist can be given several times."""
    lhelpstr="""What language is the text to proofread? For example fi_FI."""
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
        if options.wordlists == []:
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
    o, a = handleCommandLine()
    print o
    print a
    

    
