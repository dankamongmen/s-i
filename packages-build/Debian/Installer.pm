use strict;
use warnings;

package Debian::Installer;

use Debian::Debhelper::Dh_Lib;
use Exporter;
use IO::File;

use Data::Dumper;

my $configdir = "/usr/share/debian-installer/build/config";
my $dir = "/usr/share/debian-installer/build";

our @args;
our %packages;

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
      $arch = $1;
    }
    elsif (/^(XC-)?Package-Type:\s*(.*)/)
    {
      $type = $2;
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

      my $buildarch = $arch eq "all" ? $arch : get_arch();

      $packages{$package}->{filename} = $real_package . "_" . $version . "_" . $buildarch . "." . $type;

      $package = undef;
      $arch = undef;
      $type = undef;
    }
  }

  return %packages;
}

sub di_doit
{
  my $prog = shift;
  doit ($prog, @_, @args);
}

sub di_init
{
  my @ret = ();
  my $status = 1;

  my @args_input = @ARGV;
  my @args_package;
  my $arg_type;

  if (defined ($ENV{DH_OPTIONS}))
  {
    # Ignore leading/trailing whitespace.
    $ENV{DH_OPTIONS} =~ s/^\s+//;
    $ENV{DH_OPTIONS} =~ s/\s+$//;
    unshift @args_input, split (/\s+/, $ENV{DH_OPTIONS});
    undef $ENV{DH_OPTIONS};
  }

  my $i = 0;
  while ($i <= $#args_input)
  {
    my $arg = $args_input[$i];

    if ($status eq 1)
    {
      if ($arg =~ /^(-[pN]|--(package|no-package))$/)
      {
        push @args_package, $arg, $args_input[$i+1];
        $i++;
      }
      elsif ($arg =~ /^(-[pN]|--(package|no-package))/)
      {
        push @args_package, $arg;
      }
      elsif ($arg =~ /^(-[ais]|--(arch|indeb|same-arch))/)
      {
        push @args_package, $arg;
      }
      elsif ($arg =~ /^(-t|--type)$/)
      {
        $arg_type = $args_input[$i+1];
        $i++;
      }
      elsif ($arg =~ /^(-t|--type)(.+)$/)
      {
        $arg_type = $2;
      }
      elsif ($arg =~ /^--$/)
      {
        $status = 2;
        push @args, $arg;
      }
      else
      {
        push @args, $arg;
      }
    }
    else
    {
      push @args, $arg;
    }
    $i++;
  }

  control ();

  if ($arg_type)
  {
    @args_package = packages_args ($arg_type, keys %packages);
    exit 0 if $#args_package == -1;
  }

  @ARGV = @args;
  push @ARGV, @args_package;
  init ();
}

sub packages_args
{
  my $type = shift;
  my @ret;

  my $buildarch = get_arch();

  foreach (@_)
  {
    if ($type eq $packages{$_}->{type} and
        ($packages{$_}->{arch} eq "all" or
         $packages{$_}->{arch} eq "any" or
         $packages{$_}->{arch} =~ /\b$buildarch\b/))
    {
      push @ret, "-p", $_;
    }
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
