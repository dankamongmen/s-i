#! /usr/bin/perl -w

use strict;
use Unicode::String;
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

sub checkSpecials (@)
{
    my $msgid = shift;
    my $msgstr = shift;
    my $count_id = 1;
    my $count_st = 1;

    if (defined $msgstr)
    {
	return 0 if $msgid =~ /^Choose language$/ && $msgstr !~ /\/[ ]*Choose language$/;
	return 0 if $msgid =~ /^US\[/ && $msgstr !~ /^[A-Z][A-Z]$/;

	if ($_ =~ m/#. Timezones for /)
	    {
		while ($msgid =~ /,/g) { $count_id++ };
		while ($msgstr =~ /,/g) { $count_st++ };
		return 0 if ($count_id != $count_st);
	    }        

	if ($_ =~ m/#. Translators, this is a menu choice. MUST BE UNDER 65 COLUMNS/)
	    {
		utf8::decode($msgstr);
		my $lung = length $msgstr;
		return 0 if ($lung > 65);
	    }

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
        checkSpecials(@msgs) || print;
}
close (PO);
