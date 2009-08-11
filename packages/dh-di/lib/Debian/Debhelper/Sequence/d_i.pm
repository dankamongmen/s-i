#!/usr/bin/perl
# debhelper sequence file for d-i

use warnings;
use strict;
use Debian::Debhelper::Dh_Lib;

insert_after("dh_install", "dh_di_numbers");
add_command("dh_di_kernel_gencontrol", "update-control");
insert_before("dh_testdir", "dh_di_kernel_gencontrol");
insert_after("dh_install", "dh_di_kernel_install");

1;
