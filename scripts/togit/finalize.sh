#!/bin/sh

# It's easiest to fix this tag this way.
cd git/net-retriever
git tag -f v1.06 v1.06^
cd ../..

mkdir -p git/logs

(

TOP=`pwd`
for p in $(grep 'create repository ' d-i.conf | sed 's/create repository //'); do 
	echo "Finalizing $p ..."
	mv $TOP/git/$p $TOP/git/$p.git
	cd $TOP/git/$p.git
	
	for vtag in $(git tag | grep ^v); do
		# If a tag is created and then modified in svn,
		# the old version will be renamed to <tag>_<rev>
		# in git. These are clutter, so we can just remove them.
		if echo "$vtag" | egrep -q '_[0-9]+$' && [ "$vtag" != "v20031106_1" ] ; then
			printf "Old copy of changed tag found ... "
			git tag -d "$vtag"
		else
			# the tags are created with a 'v' prefix because
			# that's how svn-all-fast-export handles them properly, 
			# but we have always used just the number for tags in d-i,
			# so rename them
			tag=$(echo "$vtag" | sed 's/^v//')
			git tag "$tag" "$vtag"
			git tag -d "$vtag" >/dev/null
		fi
	done

	git gc --aggressive
	git update-server-info
done
cd $TOP

mkdir git/attic
mv git/autopartkit.git git/delo-installer.git git/kdetect.git \
	git/packages-build.git \
	git/partkit.git git/pcidetect.git git/selectdevice.git \
	git/ppcdetect.git git/pcmcia-udeb.git git/file-retriever.git \
	git/linux-modules-di-arm-2.6.git git/linux-kernel-di-arm-2.6.git \
	git/linux-modules-nonfree-di-i386-2.6.git \
	git/linux-kernel-di-i386.git \
	git/linux-kernel-di-arm.git \
	git/linux-kernel-di-hppa.git \
	git/linux-kernel-di-mips.git \
	git/linux-kernel-di-mipsel.git \
	git/linux-kernel-di-ia64.git \
	git/linux-kernel-di-sparc.git \
	git/linux-kernel-di-powerpc.git \
	git/linux-kernel-di-s390.git \
	git/linux-kernel-di-m68k.git \
	git/linux-kernel-di-alpha.git \
	git/niccfg.git git/baseconfig-udeb.git \
	git/kernel-image-di.git \
	git/sibyl-installer.git \
   git/attic/

mv log git/logs/svn-all-fast-export.log
mv git/log-* git/logs
gzip -9 git/logs/*

) 2>&1 | tee git/logs/finalize.log

(
for r in $(find git/ -maxdepth 1 -type d -name \*.git | sort); do
	repo=$(echo $r | sed 's/.git$//' | sed 's/.*\///')
	echo "[$repo]"
	echo "checkout = git clone git://hydra.kitenet.net/$repo"
	echo ""
done
) > git/d-i.mrconfig
