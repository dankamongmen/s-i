include ../../../../globalmakeflags

CFLAGS  += $(MODCFLAGS)
LDFLAGS  = $(MODLDFLAGS)

all: $(SOBJ)

$(SOBJ): $(OBJS)
	@echo Creating DSO $@ from $^
	$(CC) $(CFLAGS) -shared -o $@ $^ $(LDFLAGS)

install:
	install -d -m 755 ${moddir}/frontend
	install -m 755 $(SOBJ) ${moddir}/frontend

clean:
	-@rm -f $(SOBJ) $(OBJS) *~

