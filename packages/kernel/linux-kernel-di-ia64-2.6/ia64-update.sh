#!/bin/sh

#
# This script is used to populate the module lists for linux-kernel-di-ia64
#
# Copyright 2004 dann frazier <dannf@debian.org>
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

KVER=$1
kdir=/lib/modules/$KVER/kernel

if [ $# -ne 1 ]; then
  echo "Usage: $0 <kernel rev>"
  exit 1
fi

if [ "$(basename $(pwd))" != "ia64" ]; then
  echo "Should be called from the modules/ia64 directory"
  exit 1
fi

## md
#(cd $kdir && find drivers/md -type f) | sort > md-modules

## cdrom
#(cd $kdir && find drivers/cdrom -type f -name "*.ko" \
#    | grep -v '/cdrom.ko') | sort > cdrom-modules

modfilter() {
  sort | sed 's/\.ko/\.o/'
}

## framebuffer
(cd $kdir && find drivers/video/console/fbcon.ko) | modfilter > fb-modules

## scsi
rm -f scsi-modules
(cd $kdir && \
  find drivers/message/fusion/mptscsih.ko \
       drivers/message/fusion/mptbase.ko  \
       drivers/block/DAC960.ko  \
       drivers/block/cciss.ko  \
       drivers/block/cpqarray.ko  \
       drivers/scsi -type f | grep -v sr_mod.ko | \
       grep -v ide-scsi.ko | grep -v isense.ko) | modfilter | \
  while read mod; do
    if [ -e scsi-core-modules ] && grep -q $mod scsi-core-modules; then
      continue
    fi
    if [ -e sata-modules ] && grep -q $mod sata-modules; then
      continue
    fi

    ## ignore pcmcia storage devices
    if echo $mod | grep -q _cs.o$; then
      continue
    fi
    echo $mod | modfilter >> scsi-modules
  done

## ide
rm -f ide-modules
(cd $kdir && find drivers/ide -type f) | modfilter | while read mod; do
  if grep -q $mod ide-core-modules; then
    continue
  fi
  ## ide-cd moved to cdrom-core-modules
  if grep -q $mod cdrom-core-modules; then
    continue
  fi
  echo $mod >> ide-modules
done
(cd $kdir && find fs -type f -name isofs.ko | modfilter) >> ide-modules

## irda
rm -f irda-modules
(cd $kdir && find net/irda drivers/net/irda -type f) | modfilter > irda-modules

rm -f pcmcia-modules
(cd $kdir && find drivers/pcmcia -type f) | modfilter > pcmcia-modules

#rm -f nic-pcmcia-modules
#(cd $kdir && find drivers/net/pcmcia -type f) | modfilter > nic-pcmcia-modules

## net
#(cd $kdir && find drivers/net -name "ppp*") > ppp-modules
rm -f nic-modules
(cd $kdir && find drivers/net -type f | grep -v net/wan | \
 grep -v net/hamradio | grep -v net/fc | grep -v bsd_comp.ko | \
 grep -v net/isdn.ko | grep -v net/slip.ko | grep -v ppp_generic.ko | \
 grep -v /irda/) | modfilter | \
while read mod; do
  if grep -q $mod ipv6-modules ppp-modules plip-modules nic-shared-modules; then
    continue
  fi
  ## don't wanna pull in pcmcia stuff
  if echo $mod | grep -q _cs.o; then
    continue
  fi
  ## pulled in by ppp-modules
  if echo $mod | grep -q slhc.o; then
    continue
  fi
  echo $mod >> nic-modules
done

## if our module list matches the common one, we might want to just use it
find . -not -type l -maxdepth 1 | while read modlist; do
  if [ -e /usr/share/kernel-wedge/modules/$modlist ]; then
    cat $modlist | sort > 1
    cat /usr/share/kernel-wedge/modules/$modlist | sort > 2
    if diff -u 1 2 > /dev/null; then
      echo "==== $modlist matches the common version ===="
    fi
  fi
done
rm -f 1 2
