use strict;
use warnings;

package Debian::Installer;

use Debian::Debhelper::Dh_Lib;
use Exporter;
use IO::File;

use Data::Dumper;

my $configdir = "/usr/share/debian-installer/build/config";
my $dir = "/usr/share/debian-installer/build";

sub get_arch
{
  my $query = shift;
  $query = "DEB_HOST_ARCH" if not $query;
  $_ = `dpkg-architecture -q$query 2>/dev/null` || error($!);
  chomp;
  return $_;
}

sub config
{
  my $gnu_system = get_arch("DEB_HOST_GNU_SYSTEM");
  my $gnu_type = get_arch("DEB_HOST_GNU_TYPE");

  my %config;

  %config = config_read(\%config, "$configdir/arch/$gnu_system");
  %config = config_read(\%config, "$configdir/arch/$gnu_type");

  return %config;
}

sub config_read
{
  $_ = shift;
  my %config = %{$_};
  my $file = shift;

  my $in = IO::File -> new ($file) or return; 
  while ( <$in> )
  {
    next if m/^\s*($|#)/;
    m/^\s*(\S+)\s+(.*?)\s*$/;
    if ($1 eq "\@include")
    {
      %config = config_read(\%config, "$configdir/$2");
    }
    else
    {
      $config {$1} = $2;
    }
  }

  return %config;
}

sub control
{
  my %packages;

  my $package;
  my $arch;
  my $type;
  my $version = version();

  my $in = IO::File -> new ('debian/control') || error("cannot read debian/control: $!\n");

  while (<$in>)
  {
    chomp;
    s/\s+$//;
    if (/^Package:\s*(.*)/)
    {
      $package = $1;
    }
    elsif (/^Architecture:\s*(.*)/)
    {
      $arch = $1 eq "all" ? $1 : get_arch();
    }
    elsif (/^(XC-)?Package-Type:\s*(.*)/)
    {
      $type = $1;
    }

    if ($package and (!$_ or eof))
    {
      if (not $type)
      {
        $type = "deb";
        $type = "udeb" if $package =~ m#[-.]udeb$#;
      }
      my $real_package = $package;
      $real_package =~ s#\.udeb$##;

      $packages{$package} = {};
      $packages{$package}->{arch} = $arch,
      $packages{$package}->{package} = $package,
      $packages{$package}->{real_package} = $real_package,
      $packages{$package}->{type} = $type,

      $packages{$package}->{filename} = $real_package . "_" . $version . "_" . $arch . "." . $type;

      $package = undef;
      $arch = undef;
      $type = undef;
    }
  }

  return %packages;
}

sub fixup_arguments
{
  my @ret = ();

  my $end = 0;

  foreach (@_)
  {
    if (not $end)
    {
      if ($_ =~ /^(-[pN]|--(package|no-package))$/)
      {
        shift;
      }
      elsif ($_ =~ /^(-[pN]|--(package|no-package))/)
      { }
      elsif ($_ =~ /^(-[ais]|--(arch|indeb|same-arch))/)
      { }
      elsif ($_ =~ /^--$/)
      {
        $end = 1;
      }
      else
      {
        push @ret, $_;
      }
    }
    else
    {
      push @ret, $_;
    }
  }

  return @ret;
}

sub packages_args
{
  my $type = shift;
  my $packages = shift;
  my @ret;

  foreach (@_)
  {
    push @ret, "-p", $_ if $type eq $packages->{$_}->{type};
  }

  return @ret;
}

sub version
{
  $_ = `dpkg-parsechangelog | grep ^Version: | cut -d ' ' -f 2`;
  chomp;
  return $_;
}

1;
