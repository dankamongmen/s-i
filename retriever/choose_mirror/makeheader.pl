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
my %used_country;
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
			$used_country{$value}=1;
			$data{$site}->{$key} = $codetocountry{$value};
		}
		else {
			$data{$site}->{$key} = $value;
		}
	}
}
close IN;

# Output an array of all the countries that have mirrors. It is sorted
# lexically.
open (OUT, ">mirrors_$type.h") or die "mirrors_$type.h: $!";
print OUT "/* Automatically generated; do not edit. */\n";
print OUT "static char *countries_$type\[] = {\n";
foreach my $country (sort map $codetocountry{$_}, keys %used_country) {
	print OUT "\t\"$country\",\n";
}
# TODO: not only do we need to support i18n of this next line, but all the
# country names need to be i8n'd too (sigh).
print OUT "\t\"enter information manually\",\n";
print OUT "\tNULL\n};\n";

# Rate mirrors -- push-primary mirrors are best, primary are second best.
foreach my $site (keys %data) {
	my $rating=0;
	if (exists $data{$site}->{type}) {
		$rating=1 if $data{$site}->{type} =~ /push/i;
		$rating=2 if $data{$site}->{type} =~ /push-primary/i;
	}
	$data{$site}->{rating}=$rating;
}

# Now output the mirror list. It is ordered with better mirrors near the top.
print OUT "static struct mirror_t mirrors_$type\[] = {\n";
my $q='"';
foreach my $site (sort { $data{$b}->{rating} <=> $data{$a}->{rating} } keys %data) {
	print OUT "\t{",
		  join(", ", $q.$site.$q, $q.$data{$site}->{country}.$q,
			$q.$data{$site}->{"archive-$type"}.$q),
		  "},\n" if exists $data{$site}->{"archive-$type"};
}
print OUT "\t{NULL, NULL, NULL}\n";
print OUT "};\n";
