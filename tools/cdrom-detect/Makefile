
all:

install: 
	# Copy in limited autodetect databases to save space.
	install -d $(DESTDIR)/usr/share/detect 
	for detectdb in /usr/share/detect/*.lst; \
	do \
	    detectfn=`basename $$detectdb`; \
	    cat $$detectdb | \
        	awk 'BEGIN { FS = "\t"; CURRENTVENDOR = ""; LASTVENDOR = "Nonexistent Vendor" }; /^[^\t]/ { CURRENTVENDOR = $$0 }; /^\t/ && $$3 ~ /(scsi|ether|cdrom)/ { if (CURRENTVENDOR != LASTVENDOR) print CURRENTVENDOR; print $$0; LASTVENDOR = CURRENTVENDOR }' \
        	> $(DESTDIR)/usr/share/detect/$$detectfn ; \
	done
	#rm $(DESTDIR)/usr/share/detect/isa.lst
	#touch $(DESTDIR)/usr/share/detect/isa.lst
	
