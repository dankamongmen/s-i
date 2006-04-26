#! /usr/bin/perl -w

use strict;
use Unicode::String;

sub checkSpecials (@)
{
    my $msgid = shift;
    my $msgstr = shift;
    my $count_id = 0;
    my $count_st = 0;

    if (defined $msgstr)
    {
	return 0 if $msgid =~ /^Choose language$/ && $msgstr !~ /\/[ ]*Choose language$/;
	return 0 if $msgid =~ /^US\[/ && $msgstr !~ /^[A-Z][A-Z]$/;

	if ($_ =~ /#.\s+Type:\s+select/ && $_ =~ /#.\s+Choices/ )
	    {
		while ($msgid =~ /,/g) { $count_id++ };
		if ($count_id > 0)
		{
		    while ($msgstr =~ /,/g) { $count_st++ };
		    return 0 if ($count_id != $count_st);
		}
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
        checkSpecials(@msgs) || print;
}
close (PO);
