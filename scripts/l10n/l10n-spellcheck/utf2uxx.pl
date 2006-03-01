#! /usr/bin/perl -C1
#
# Written by Denis Barbier
#

sub c {
	my $text = shift;
	my $convert_ascii = shift;
	my $ret = '';
	while ($text =~ s/(.)//) {
		$l = unpack("U", $1);
		if ($convert_ascii == 0 && $l < 0x80) {
			$ret .= $1;
		} else {
			$ret .= sprintf "<U%04X>", $l;
		}
	}
	return $ret;
}

my $convert_ascii = 1;
while (<>) {
	if (/^LC_IDENTIFICATION/) {
		$convert_ascii = 0;
	} elsif (/^END LC_IDENTIFICATION/) {
		$convert_ascii = 1;
	}
	my $conv = $convert_ascii;
	$conv = 0 if (/^(copy|include)/);
	s/"([^"]*)"/'"'.c($1, $conv).'"'/eg;
	print;
}
