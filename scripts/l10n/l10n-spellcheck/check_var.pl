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

sub checkVars (@)
{
        my $msgid = shift;
        my $var1 = getVars($msgid);

	# Exceptions:
	# - in kbd-chooser KBD-ARCHS should be translated with KBD-ARCHS-L10N
	# - in tasksel ORIGCHOICES should be translated with CHOICES
	$var1 =~ s/KBD-ARCHS/KBD-ARCHS-L10N/;
	$var1 =~ s/ORIGCHOICES/CHOICES/;

	for (@_)
        {
                my $var2 = getVars($_);
                return 0 if $var1 ne $var2;
        }
        return 1;
}

$/ = "\n\n";
open (PO, "< $ARGV[0]") or die "Unable to open $ARGV[0]: $!\n";
while (<PO>)
{
        s/"\n"//g;
        next if m/^#, .*fuzzy/m;
        next unless m/^msgid ".+"$/m;
        my @msgs = ();
        while (m/^msg\S+ "(.+)"$/mg)
        {
                push (@msgs, $1);
        }
        checkVars(@msgs) || print;
}
close (PO);
