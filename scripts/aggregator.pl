#!/usr/bin/perl
# Perl module used by the aggregator programs.

use warnings;
use strict;

sub aggregate {
	foreach my $log (@_) {
		print "<li><a href=\"".$log->{url}."\">".$log->{description}."</a><br>\n";
		my $logurl=$log->{logurl}."overview".$log->{logext}."\n";
		if ($logurl=~m#.*://#) {
			if (! open (LOG, "wget --tries=3 --timeout=5 --quiet -O - $logurl |")) {
				print "<b>wget error</b>\n";
			}
		}
		else {
			open (LOG, $logurl) || print "<b>cannot open $logurl</b>\n";
		}
		my @lines=<LOG>;
		if (! close LOG) {
			print "<b>failed</b> to download <a href=\"$logurl\">summary log</a>";
			next;
		}
		if (! @lines) {
			print "<b>empty</b> <a href=\"$logurl\">log</a>";
		}
		else {
			print "<ul>\n";
		}
		foreach my $line (@lines) {
			chomp $line;
			my ($arch, $date, $builder, $ident, $status, $notes) = 
				$line =~ /^(.*?)\s+\((.*?)\)\s+(.*?)\s+(.*?)\s+(.*?)(?:\s+(.*))?$/;
			if (! defined $status) {
				print "<li><b>unparsable</b> entry:</i> $line\n";
			}
			else {
				$date=~s/[^A-Za-z0-9: ]//g; # untrusted string
				my $shortdate=`TZ=GMT LANG=C date -d '$date' '+%b %d %H:%M'`;
				chomp $shortdate;
				if (! length $shortdate) {
					$shortdate=$date;
				}
				my $unixdate=`TZ=GMT LANG=C date -d '$date' '+%s'`;
				if (length $unixdate && 
				    (time - $unixdate) > ($log->{frequency}+1) * 60*60*24)  {
					$shortdate="<b>$shortdate</b>";
				}
				if ($status eq 'failed') {
					$status='<b>failed</b>';
				}
				if (defined $notes && length $notes) {
					$notes="($notes)";
				}
				else {
					$notes="";
				}
				print "<li>$arch $shortdate $builder <a href=\"".$log->{logurl}."$ident".$log->{logext}."\">$status</a> $ident $notes\n";
			}
		}
		print "</ul>\n";
	}
}

1
