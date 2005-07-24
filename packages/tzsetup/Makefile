all: tzmap debian/common.templates
tzmap: gen-templates debian/common.templates.in /usr/share/zoneinfo/zone.tab
	./gen-templates < debian/common.templates.in > debian/common.templates
debian/common.templates: gen-templates debian/common.templates.in /usr/share/zoneinfo/zone.tab
	./gen-templates < debian/common.templates.in > debian/common.templates

clean:
	rm -f tzmap
