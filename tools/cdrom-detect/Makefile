
all:

install: 
	# Copy in limited autodetect databases to save space.
	install -d $(DESTDIR)/usr/share/discover
	for discoverdb in /usr/share/discover/*.lst; \
	do \
	    discoverfn=`basename $$discoverdb`; \
	    cat $$discoverdb | \
        	awk 'BEGIN { FS = "\t"; CURRENTVENDOR = ""; LASTVENDOR = "Nonexistent Vendor" }; /^[^\t]/ { CURRENTVENDOR = $$0 }; /^\t/ && $$3 ~ /(scsi|ether|cdrom)/ { if (CURRENTVENDOR != LASTVENDOR) print CURRENTVENDOR; print $$0; LASTVENDOR = CURRENTVENDOR }' \
        	> $(DESTDIR)/usr/share/discover/$$discoverfn ; \
	done
	#rm $(DESTDIR)/usr/share/discover/isa.lst
	#touch $(DESTDIR)/usr/share/discover/isa.lst

clean:
