docdir = $(datadir)/doc/$(PACKAGE)$(LIBRARY_VERSION_MAJOR)-dev

doc html:
	$(DOXYGEN)

install-data-local:
	@if [ -e html ]; then \
	  $(MAKE) install-real-html; \
	fi

clean-local:
	rm -rf html latex

install-real-html: html
	$(mkinstalldirs) $(DESTDIR)$(docdir)/html
	@list='$(wildcard html/*)'; for p in $$list; do \
	  echo " $(INSTALL_DATA) $$p $(DESTDIR)$(docdir)/$$p"; \
	  $(INSTALL_DATA) $$p $(DESTDIR)$(docdir)/$$p; \
	done

