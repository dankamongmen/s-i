#! /usr/bin/perl -w

use strict;
use Getopt::Long qw(GetOptions);

my $sort = 0;
GetOptions(
        'sort|s' => \$sort
);

sub getVars
{
        my $text = shift;
        my @vars = ();
        while ($text =~ m/\G.*?(\$\{[^{}]+\})/g) {
                push @vars, $1;
        }
        @vars = sort (@vars) if $sort;
        return join ('', @vars);
}

$/ = "\n\n";
open (PO, "< $ARGV[0]") or die "Unable to open $ARGV[0]: $!\n";
while (<PO>)
{
        s/"\n"//g;
        next if m/^#, .*fuzzy/m;
        next unless m/^msgid/m;
        (my $msgid) = m/^msgid "(.*)"$/m;
        (my $msgstr) = m/^msgstr "(.*)"$/m;
        next if $msgstr eq '';
        my $var1 = getVars($msgid);
        my $var2 = getVars($msgstr);
        print if $var1 ne $var2;
}
close (PO);
