#!/usr/bin/perl -s
#we added -s to have simple params functionality

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Library General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#Author: Eddy Petrisor <eddy.petrisor@gmail.com>
#
#Call the script with parameter -help to see the options.


sub setup()
{
	$needhelp = 1;
	if ($msgid) {$msgmark="msgid"; $needhelp = 0;}
	if ($msgstr) {$msgmark="msgstr"; $needhelp = 0;}
	if ($noignore)
	{
		if ($msgmark) {$firsttime = 0;}
		else { $needhelp = 1; }
	}
	unless ($msgmark) { $needhelp = 1; }
	if ( ($help) || ($needhelp eq 1) ) { help(); }
}

sub help()
{
	print "\n\nUsage: $0 [-msgid|-msgstr [-noignore]\n";
	print "\nExtracts msgid or msgstr from a .po file.\n";
	print "\t-msgid\n\t\textract mesgid-s\n\t-msgstr\n\t\textract msgstr-s\n";
	print "\t-noignore\n\t\tby default, the first msgid/msgstr is ignored because";
	print "\n\t\tregular .po files have the header in there. If this";
	print "\n\t\tparameter is specified then the first msgid/msgstr is";
	print "\n\t\tincluded, too.\n";
	print "\n\nExample: cat ro.po | perl -s $0 -msgid >ro_all.txt\n";
	print "\nNOTE: This script needs to be run with Perl's -s option.\n\n";
	die "\n";
}

$ismsg = 0;
$firsttime = 1;

setup();

while (defined($line = <STDIN>)) 
{
	# process $line here
	unless ( $line =~ /\s*#.*/ )
	{
		if ( $line =~ /^$msgmark .*/)
		{
			$line =~ s/^$msgmark (.*)/\1/;
			unless ($line =~ /^\"\"/) { print "- $line";}
			#do not parse the header, if requested not to.
			if ($firsttime == 0) {$ismsg = 1;}
			$firsttime = 0;
		}
		else 
		{
			if ($ismsg eq 1)
			{
				if ( $line =~ /^\s*".+"/ )
				{
					print "- $line";
				}
				else
				{
					$ismsg = 0;
				}
			}
		}
	}
}
