all: tzmap debian/common.templates
iso_3166.tab:
	isoquery | cut -f 1,4 | sort > iso_3166.tab
tzmap: iso_3166.tab gen-templates debian/common.templates.in /usr/share/zoneinfo/zone.tab
	./gen-templates < debian/common.templates.in > debian/common.templates
debian/common.templates: gen-templates debian/common.templates.in /usr/share/zoneinfo/zone.tab
	./gen-templates < debian/common.templates.in > debian/common.templates

clean:
	rm -f tzmap iso_3166.tab
