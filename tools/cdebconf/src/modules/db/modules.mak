include ../../../../globalmakeflags

CFLAGS  += -I../../.. $(MODCFLAGS)
LDFLAGS  = $(MODLDFLAGS)

all: $(SOBJ)

$(SOBJ): $(OBJS)
	@echo Creating DSO $@ from $^
	@$(CC) $(CFLAGS) -shared -o $@ $^ $(LDFLAGS)

install:
	install -d -m 755 ${moddir}/db
	install -m 644 $(SOBJ) ${moddir}/db

clean:
	-@rm -f $(SOBJ) $(OBJS) *~
