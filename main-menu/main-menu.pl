#!/usr/bin/perl
# Sometimes, it's so much easier to use perl.

$/="\n\n";
while (<>) {
	$package=$1 if /Package: (.*?)\n/;
	$depends=$1 if /Depends: (.*?)\n/;
	$installer_menu_item=$1 if /Installer-Menu-Item: (\d+)/;
	$provides=$1 if /Provides: (.*?)\n/;
	$deps{$package}=[split(/, /, $depends)];
	$provides{$package}=$provides;
	$descriptions{$package}=$1 if /Description: (.*?)\n/;
	$installer_menu_item{$package}=$installer_menu_item;
	$exists{$package}=1;
	$desc=$package=$depends=$installer_menu_item=$provides='';
}

my %seen;

sub recursive_fullfill {
	my $pkg=shift;
	next if $seen{$pkg};
	foreach my $dep (@{$deps{$pkg}}) {
		next if $seen{$dep};
		# Find a set of candidates that can fullfill this
		# dependancy. That is all packages that provide it, plus
		# the package itself if it really exists.
		my @candidates=();
		push @candidates, $dep if $exists{$dep};
		foreach (keys %provides) {
			push @candidates, $_ if $provides{$_} eq $dep;
		}
		foreach (@candidates) {
			recursive_fullfill($_);
		}
	}
	$seen{$pkg}=1;
	print "--$descriptions{$pkg}\n" if $installer_menu_item{$pkg} ne '';
}

# Partial trees work too..
#recursive_fullfill('install-base');

foreach $pkg (sort {
	         $installer_menu_item{$a} <=> $installer_menu_item{$b}
                                          ||
 	                               $a cmp $b
 	       } keys %installer_menu_item) {
	recursive_fullfill($pkg) if $installer_menu_item{$pkg};
}
