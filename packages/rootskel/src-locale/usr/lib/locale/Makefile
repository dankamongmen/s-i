dir = usr/lib/locale

build-local:: C.UTF-8

C.UTF-8: C.UTF-8.in
	localedef -i $^ -f "UTF-8" ./$@

install-local:: C.UTF-8
	cp -a $< $(outdir)

clean-local::
	rm -rf C.UTF-8

include ../../../../Makefile.inc
