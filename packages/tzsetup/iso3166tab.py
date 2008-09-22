#!/usr/bin/python
#
# Read iso-codes data file and output a .tab file
#
# Copyright (C) 2004 Alastair McKinstry <mckinstry@debian.org>
# Released under the GPL.
# $Id: iso3166tab.py,v 1.2 2005/01/08 18:06:46 mckinstry Exp $

from xml.sax import _exceptions, handler, make_parser, SAXException, ContentHandler
from xml.sax.handler import feature_namespaces
import sys, os, getopt, urllib2

class ErrorPrinter:
    "A simple class that just prints error messages to standard out."

    def __init__(self, level=0, outfile=sys.stderr):
        self._level = level
        self._outfile = outfile

    def warning(self, exception):
        if self._level <= 0:
            self._outfile.write("WARNING in %s: %s\n" %
                               (self.__getpos(exception),
                                exception.getMessage()))

    def error(self, exception):
        if self._level <= 1:
            self._outfile.write("ERROR in %s: %s\n" %
                               (self.__getpos(exception),
                                exception.getMessage()))

    def fatalError(self, exception):
        if self._level <= 2:
            self._outfile.write("FATAL ERROR in %s: %s\n" %
                               (self.__getpos(exception),
                                exception.getMessage()))

    def __getpos(self, exception):
        if isinstance(exception, _exceptions.SAXParseException):
            return "%s:%s:%s" % (exception.getSystemId(),
                                 exception.getLineNumber(),
                                 exception.getColumnNumber())
        else:
            return "<unknown>"

class DefaultHandler(handler.EntityResolver, handler.DTDHandler,
                     handler.ContentHandler, handler.ErrorHandler):
    """Default base class for SAX2 event handlers. Implements empty
    methods for all callback methods, which can be overridden by
    application implementors. Replaces the deprecated SAX1 HandlerBase
    class."""


class printLines(DefaultHandler):
	def __init__(self, ofile):
		self.ofile = ofile

	def startElement(self, name, attrs):
		if name != 'iso_3166_entry':
			return
		code = attrs.get('alpha_2_code', None)
		if code == None:
			raise RunTimeError, "Bad file"
		if type(code) == unicode:
			code = code.encode('UTF-8')
		name = attrs.get('name', None)
		if name == None:
			raise RunTimeError, " BadFile"
		if type(name) == unicode:
			name = name.encode('UTF-8')
		common_name = attrs.get('common_name', None)
		if common_name != None:
			if type(common_name) == unicode:
				name = common_name.encode('UTF-8')
			else:
				name = common_name
		self.ofile.write (code + '\t' + name + '\n')


##
## MAIN
##


ofile = sys.stdout
p = make_parser()
p.setErrorHandler(ErrorPrinter())
try:
	dh = printLines(ofile)
	p.setContentHandler(dh)
	p.parse(sys.argv[1])
except IOError,e:
	print in_sysID+": "+str(e)
except SAXException,e:
	print str(e)

ofile.close()
