include ../../../../globalmakeflags

CFLAGS  += $(MODCFLAGS)
LDFLAGS  = $(MODLDFLAGS)
SUBDIR   = src/modules/db/$(MODULE)

all: $(SOBJ)

$(SOBJ): $(OBJS)
	@echo Creating DSO $@ from $^
	@$(CC) $(CFLAGS) -shared -o $@ $^ $(LDFLAGS)

install:
	install -d -m 755 ${moddir}/db
	install -m 755 $(SOBJ) ${moddir}/db

clean:
	-@rm -f $(SOBJ) $(OBJS) *~
