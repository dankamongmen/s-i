#!/usr/bin/perl -w
# Generates a mirrors_<type>.h file, reading from Mirrors.masterlist.
# Note that there will be duplicate strings in the generated file.
# I am relying on the c compiler to fix this, which gcc does.
#
# Pass in the type of mirror we are interested in (http or ftp).
use strict;

my $type = shift || die "please specify mirror type\n";

# Read in the zoneinfo file to map country codes to names.
my $zoneinfo = '/usr/share/zoneinfo/iso3166.tab';
open (IN, $zoneinfo) or die "$zoneinfo: $!";
my %codetocountry;
while (<IN>) {
	chomp;
	next if /^\s*#/;
	my ($code, $country) = split(' ', $_, 2);
	$codetocountry{lc $code} = $country;
}
close IN;

# Slurp in the mirror file.
my %data;
my $site;
open (IN, "Mirrors.masterlist") or die "Mirrors.masterlist: $!";
while (<IN>) {
	chomp;
	if (m/([^:]*):\s+(.*)/) {
		my $key = lc $1;
		my $value = $2;
		if (lc $key eq 'site') {
			$site = $value;
		}
		elsif (lc $key eq 'country') {
			$value =~ s/ .*//;
			$value = lc $value;
			$data{$site}->{$key} = $codetocountry{$value};
		}
		else {
			$data{$site}->{$key} = $value;
		}
	}
}
close IN;

open (OUT, ">mirrors_$type.h") or die "mirrors_$type.h: $!";
print OUT "/* Automatically generated; do not edit. */\n";

# Poor man's mirror rating system: push-primary, push* (-secondary), others
foreach my $site (keys %data) {
	my $rating=0;
	if (exists $data{$site}->{type}) {
		$rating=1 if $data{$site}->{type} =~ /push/i;
		$rating=2 if $data{$site}->{type} =~ /push-primary/i;
	}
	$data{$site}->{rating}=$rating;
}

# Now output the mirror list. It is ordered with better mirrors near the top.
my %used_country;
print OUT "static struct mirror_t mirrors_$type\[] = {\n";
my $q='"';
foreach my $site (sort { $data{$b}->{rating} <=> $data{$a}->{rating} } keys %data) {
	next unless exists $data{$site}->{"archive-$type"} and
		    exists $data{$site}->{country};
	print OUT "\t{",
		  join(", ", $q.$site.$q, $q.$data{$site}->{country}.$q,
			$q.$data{$site}->{"archive-$type"}.$q),
		  "},\n";
	$used_country{$data{$site}->{country}}=1;
}
print OUT "\t{NULL, NULL, NULL}\n";
print OUT "};\n";

# Output an array of all the countries that have mirrors of the given type. 
# It is sorted lexically.
print OUT "static char *countries_$type\[] = {\n";
foreach my $country (sort keys %used_country) {
	print OUT "\t\"$country\",\n";
}
# TODO: not only do we need to support i18n of this next line, but all the
# country names need to be i18n'd too (sigh).
print OUT "\t\"enter information manually\",\n";
print OUT "\tNULL\n};\n";

close OUT;
