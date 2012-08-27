TARGETS=$(foreach language,$(languages),$(addprefix $(language).,$(architectures)))

.PHONY: $(TARGETS)

all: $(TARGETS)

$(TARGETS): %:
	target=$@ ; \
	lang=$${target%.*} ; \
	arch=$${target#*.} ; \
	echo "Architecture: $$arch" ; \
	if [ -n "$$noarchdir" ]; then \
		destsuffix="$$lang" ; \
	else \
		if [ -n "$$web" ] ; then \
			destsuffix="$$arch" ; \
		else \
			destsuffix="$$lang.$$arch" ; \
		fi ; \
	fi ; \
	export destdir=build.out.$$lang.$$arch ; \
	export tempdir=build.tmp.$$lang.$$arch ; \
	./buildone.sh "$$arch" "$$lang" "$$formats" ; \
	mkdir -p "$$destination/$$destsuffix" ; \
	for format in $$formats; do \
		if [ "$$format" = html ]; then \
			mv ./$$destdir/html/* "$$destination/$$destsuffix" ; \
		else \
			# Do not fail because of missing PDF support for some languages \
			if [ -n "$$web" ] ; then \
				mv ./$$destdir/install.$$lang.$$format "$$destination/$$destsuffix/install.$$format.$$lang" 2>/dev/null || true ; \
			else \
				mv ./$$destdir/install.$$lang.$$format "$$destination/$$destsuffix" 2>/dev/null || true ; \
			fi ; \
		fi ; \
	done ; \
	./clear.sh
