#! /usr/bin/perl

sub getVars
{
        my $text = shift;
        my $var = '';
        while ($text =~ m/\G.*?(\$\{[^{}]+\})/g) {
                $var .= $1;
        }
        return $var;
}

$/ = "\n\n";
open (PO, "< $ARGV[0]") or die "Unable to open $ARGV[0]: $!\n";
while (<PO>)
{
        s/"\n"//g;
        (my $msgid) = m/^msgid "(.*)"$/m;
        (my $msgstr) = m/^msgstr "(.*)"$/m;
        next if $msgstr eq '' || m/^#, .*fuzzy/m;
        my $var1 = getVars($msgid);
        my $var2 = getVars($msgstr);
        print if $var1 ne $var2;
}
close (PO);
