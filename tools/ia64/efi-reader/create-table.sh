#!/bin/sh

# Build iso-639-2 to iso-639-1 table for efi-reader
# 
#

ISO_TABLE=/usr/share/iso-codes/iso-639.tab

cat > table.h << EOF

struct {
	char threecode[4];
	char twocode[3];
	} trans_table[] = {
EOF

egrep -v "^#.*" $ISO_TABLE | egrep -v "XX"  | \
sed -r 's/([a-z]+)\t\w+\t([a-z]+)\t.*$/{"\1"\,"\2"}\,/' \
>> table.h


cat >> table.h << EOF
 { "","" },
};

EOF
