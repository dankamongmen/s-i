include ../../../../globalmakeflags

CFLAGS  += -I../../.. $(MODCFLAGS)
LDFLAGS  = $(MODLDFLAGS)

all: $(SOBJ)

$(SOBJ): $(OBJS)
	@echo Creating DSO $@ from $^
	@$(CC) $(CFLAGS) -shared -o $@ $^ $(LDFLAGS)

install:
	install -d -m 755 ${moddir}/frontend
	install -m 644 $(SOBJ) ${moddir}/frontend

clean:
	-@rm -f $(SOBJ) $(OBJS) *~
