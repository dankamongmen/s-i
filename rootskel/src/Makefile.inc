outdir = $(DESTDIR)/$(dir)

install: install-local install-recursive

install-local: $(outdir) install-local-files install-local-files-exec

install-local-files: $(files)
	@if [ -z "$(DESTDIR)" ]; then \
	  echo "You *must* give a DESTDIR!"; \
	  exit 1; \
	fi
	@list='$(files)'; for file in $$list; do \
	  echo "install -m644 $$file $(outdir)"; \
	  install -m644 $$file $(outdir); \
	done

install-local-files-exec: $(files_exec)
	@if [ -z "$(DESTDIR)" ]; then \
	  echo "You *must* give a DESTDIR!"; \
	  exit 1; \
	fi
	@list='$(files_exec)'; for file in $$list; do \
	  echo "install -m755 $$file $(outdir)"; \
	  install -m755 $$file $(outdir); \
	done

install-recursive: $(outdir)
	@if [ -z "$(DESTDIR)" ]; then \
	  echo "You *must* give a DESTDIR!"; \
	  exit 1; \
	fi
	@list='$(subdirs)'; for subdir in $$list; do \
	  echo "Making install in $$subdir"; \
	  (cd $$subdir && $(MAKE) install) \
	done

clean: clean-local clean-recursive

clean-local::

clean-recursive:
	@list='$(subdirs)'; for subdir in $$list; do \
	  echo "Making clean in $$subdir"; \
	  (cd $$subdir && $(MAKE) clean) \
	done

$(outdir):
	install -d $(outdir)

