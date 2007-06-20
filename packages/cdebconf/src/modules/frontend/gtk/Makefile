MODULE=gtk
SOBJ=gtk.so
OBJS=gtk.opic
MODCFLAGS=`pkg-config --cflags gtk+-directfb-2.0 gthread-2.0 directfb-internal`

# let the type char be unsigned by default
MODCFLAGS += -funsigned-char
# apply strict aliasing rules
MODCFLAGS += -fstrict-aliasing
# turn on all common warnings
MODCFLAGS += -Wall
# turn on extra warnings
MODCFLAGS += -W
# treat warnings as errors
MODCFLAGS += -Werror
# warn about undefined preprocessor identifiers
MODCFLAGS += -Wundef
# warn about casting of pointers to increased alignment requirements
MODCFLAGS += -Wcast-align
# make string constants const
MODCFLAGS += -Wwrite-strings
# warn about comparisons between signed and unsigned values
MODCFLAGS += -Wsign-compare
# warn about unused declared stuff
MODCFLAGS += -Wno-unused-parameter
# warn about variables which are initialized with themselves
MODCFLAGS += -Winit-self
# warn about pointer arithmetic on void* and function pointers
MODCFLAGS += -Wpointer-arith
# warn about multiple declarations
MODCFLAGS += -Wredundant-decls
# do not warn about zero-length formats.
MODCFLAGS += -Wno-format-zero-length
# missing prototypes
MODCFLAGS += -Wmissing-prototypes
# warn about functions without format attribute that should have one
MODCFLAGS += -Wmissing-format-attribute

MODLDFLAGS=`pkg-config --libs gtk+-directfb-2.0 gthread-2.0 directfb-internal`

include ../modules.mak
