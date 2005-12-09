#!/usr/bin/perl
# Perl module used by the aggregator programs.

use warnings;
use strict;

sub aggregate {
	my $fh=shift;
	
	my $success=0;
	my $failed=0;
	my $total=0;
	my $old=0;
	
	foreach my $log (@_) {
		print $fh "<li><a href=\"".$log->{url}."\">".$log->{description}."</a><br>\n";
		my $logurl=$log->{logurl}."overview".$log->{logext}."\n";
		if ($logurl=~m#.*://#) {
			if (! open (LOG, "wget --tries=3 --timeout=5 --quiet -O - $logurl |")) {
				print $fh "<b>wget error</b>\n";
			}
		}
		else {
			open (LOG, $logurl) || print $fh "<b>cannot open $logurl</b>\n";
		}
		my @lines=<LOG>;
		if (! close LOG) {
			print $fh "<b>failed</b> to download <a href=\"$logurl\">summary log</a>";
			next;
		}
		if (! @lines) {
			print $fh "<b>empty</b> <a href=\"$logurl\">log</a>";
		}
		else {
			print $fh "<ul>\n";
		}
		foreach my $line (@lines) {
			chomp $line;
			my ($arch, $date, $builder, $ident, $status, $notes) = 
				$line =~ /^(.*?)\s+\((.*?)\)\s+(.*?)\s+(.*?)\s+(.*?)(?:\s+(.*))?$/;
			if (! defined $status) {
				print $fh "<li><b>unparsable</b> entry:</i> $line\n";
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
					$old++;
				}
				if ($status eq 'failed') {
					$status='<b>failed</b>';
					$failed++;
				}
				elsif ($status eq 'success') {
					$success++;
				}
				$total++;
				if (defined $notes && length $notes) {
					$notes="($notes)";
				}
				else {
					$notes="";
				}
				print $fh "<li>$arch $shortdate $builder <a href=\"".$log->{logurl}."$ident".$log->{logext}."\">$status</a> $ident $notes\n";
			}
		}
		print $fh "</ul>\n";
	}

	return ($total, $failed, $success, $old);
}

1
