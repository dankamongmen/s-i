#!/bin/sh
nsgmls -sv -c /usr/share/sgml/docbook/custom/simple/catalog /usr/share/sgml/declaration/xml.dcl $1 2> errors
