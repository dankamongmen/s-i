
all:

install: lst
	install -d $(DESTDIR)/usr/share/discover
	for shortdb in *.lst ; \
	do \
	    cp $$shortdb $(DESTDIR)/usr/share/discover/$$shortdb ; \
	done
	#rm $(DESTDIR)/usr/share/discover/isa.lst
	#touch $(DESTDIR)/usr/share/discover/isa.lst

lst: /usr/share/discover/*.lst
	# Copy in limited autodetect databases to save space.
	for discoverdb in /usr/share/discover/*.lst ; \
	do \
	    discoverfn=`basename $$discoverdb`; \
	    cat $$discoverdb | \
        	awk 'BEGIN { FS = "\t"; CURRENTVENDOR = ""; LASTVENDOR = "Nonexistent Vendor" }; /^[^\t]/ { CURRENTVENDOR = $$0 }; /^\t/ && $$3 ~ /(scsi|ether|cdrom)/ { if (CURRENTVENDOR != LASTVENDOR) print CURRENTVENDOR; print $$0; LASTVENDOR = CURRENTVENDOR }' \
        	> $$discoverfn ; \
	done
	
clean:
	$(RM) *.lst
