include ../../../../globalmakeflags

CFLAGS  += $(MODCFLAGS)
LDFLAGS  = $(MODLDFLAGS)

all: $(SOBJ)

$(SOBJ): $(OBJS)
	@echo Creating DSO $@ from $^
	@$(CC) $(CFLAGS) -shared -o $@ $^ $(LDFLAGS)

install:
ifneq ($(INSTALLOBJ),)
	install -d -m 755 ${moddir}/db
	install -m 644 $(INSTALLOBJ) ${moddir}/db
endif

clean:
	-@rm -f $(SOBJ) $(OBJS) *~
