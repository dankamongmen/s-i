#!/usr/bin/perl -w
# Generates a mirrors_<type>.h file, reading from Mirrors.masterlist.
# Note that there will be duplicate strings in the generated file.
# I am relying on the c compiler to fix this, which gcc does.
#
# Pass in the type of mirror we are interested in (http or ftp), or
# 'template' to generate templates-countries file
use strict;

my $type = shift || die "please specify mirror type\n";

# Slurp in the mirror file.
my %data;
my %countries;
my %http_countries;
my %ftp_countries;
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
			$value = uc $value;
			$data{$site}->{$key} = $value;
		}
		else {
			$data{$site}->{$key} = $value;
		}
	}
}
close IN;

# Poor man's mirror rating system: push-primary, push* (-secondary), others
foreach my $site (keys %data) {
	my $rating=0;
	if (exists $data{$site}->{type}) {
	        $rating=1 if $data{$site}->{type} =~ /push/i;
                $rating=2 if $data{$site}->{type} =~ /push-primary/i;
        }
       $data{$site}->{rating}=$rating;
}
if ($type eq 'template') { 
 	open (TEMPLATE, ">debian/templates-countries") or die "debian/templates-countries: $!";
	foreach my $site (sort keys %data) {
		next unless exists $data{$site}->{"archive-http"} and
		                    exists $data{$site}->{country};
		$http_countries{$data{$site}->{country}} = 1;
	}
	foreach my $site (sort keys %data) {
		 next unless exists $data{$site}->{"archive-ftp"} and
                      exists $data{$site}->{country};
                $ftp_countries{$data{$site}->{country}} = 1;
	}
	print TEMPLATE "Template: mirror/http/countries\n";
        print TEMPLATE "Type: select\n";
	print TEMPLATE "#  [NOTE] Translate the ISO country codes below into localised country names.\n";
        print TEMPLATE "__Choices: enter information manually";
	foreach  my $country (sort (keys %http_countries)) {
		print TEMPLATE ", ${country}";
	}
	print TEMPLATE "\n";
	print TEMPLATE "#  Translators, you must not translate this value, but you can change it\n".
"#  to one of those listed above, e.g. msgstr \"GB\".  Square brackets are\n".
"#  ignored and appear here only to distinguish this msgid from the same\n".
"#  one in the Choices field.\n";
	print TEMPLATE "_Default: US[ Default value for http]\n";
	print TEMPLATE "_Description: Debian archive mirror country:\n";
	print TEMPLATE " The goal is to find a mirror of the Debian archive that is close to you on the network -- be\n";
	print TEMPLATE " aware that nearby countries, or even your own, may not be the best choice.\n\n";

	print TEMPLATE "Template: mirror/ftp/countries\n";
	print TEMPLATE "Type: select\n";
	print TEMPLATE "#  [NOTE] Translate the ISO country codes below into localised country names.\n";
        print TEMPLATE "__Choices: enter information manually";
	foreach  my $country (sort (keys %ftp_countries)) {
                print TEMPLATE ", ${country}";
        }
	print TEMPLATE "\n";
	print TEMPLATE "#  Translators, you must not translate this value, but you can change it\n".
"#  to one of those listed above, e.g. msgstr \"GB\".  Square brackets are\n".
"#  ignored and appear here only to distinguish this msgid from the same\n".
"#  one in the Choices field.\n";
	print TEMPLATE "_Default: US[ Default value for ftp]\n";
	print TEMPLATE "_Description: Debian archive mirror country:\n";
	print TEMPLATE " The goal is to find a mirror of the Debian archive that is close to you on the network -- be\n";
        print TEMPLATE " aware that nearby countries, or even your own, may not be the best choice.\n\n";
	close TEMPLATE;
	exit 0;
}

if ($type eq 'httplist') { 
 	open (LIST, ">debian/httplist-countries") or die "debian/httplist-countries: $!";
	foreach my $site (sort keys %data) {
		next unless exists $data{$site}->{"archive-http"} and
		                    exists $data{$site}->{country};
		$countries{$data{$site}->{country}} = 1;
	}
	foreach  my $country (sort (keys %countries)) {
		print LIST "${country}\n";
	}
	close LIST;
	exit 0;
}

if ($type eq 'ftplist') { 
 	open (LIST, ">debian/ftplist-countries") or die "debian/ftplist-countries: $!";
	foreach my $site (sort keys %data) {
		next unless exists $data{$site}->{"archive-ftp"} and
		                    exists $data{$site}->{country};
		$countries{$data{$site}->{country}} = 1;
	}
	foreach  my $country (sort (keys %countries)) {
		print LIST "${country}\n";
	}
	close LIST;
	exit 0;
}

open (OUT, ">mirrors_$type.h") or die "mirrors_$type.h: $!";
print OUT "/* Automatically generated; do not edit. */\n";

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
# 	$used_country{$data{$site}->{country}}=1;
}
print OUT "\t{NULL, NULL, NULL}\n";
print OUT "};\n";

close OUT;
