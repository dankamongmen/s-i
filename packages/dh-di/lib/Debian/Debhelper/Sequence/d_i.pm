#!/usr/bin/perl
# debhelper sequence file for d-i

use warnings;
use strict;
use Debian::Debhelper::Dh_Lib;

insert_after("dh_install", "dh_di_numbers");

1;
