include ../../../../globalmakeflags

CFLAGS  += -I../../.. $(MODCFLAGS)
LDFLAGS  = $(MODLDFLAGS)

all: $(SOBJ)

$(SOBJ): $(OBJS)
	@echo Creating DSO $@ from $^
	@$(CC) $(CFLAGS) -shared -o $@ $^ $(LDFLAGS)

clean:
	-@rm -f $(SOBJ) $(OBJS) *~
