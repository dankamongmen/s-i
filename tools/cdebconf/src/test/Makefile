include ../../globalmakeflags
CFLAGS += -I..
BIN=configtest dbtest
OBJS=$(subst .c,.o,$(wildcard *.c))

all: $(BIN)

configtest: ../strutl.o ../configuration.o configtest.o
	$(CC) -o $@ $^

dbtest: ../strutl.o ../configuration.o ../priority.o ../template.o ../question.o ../database.o dbtest.o
	$(CC) -o $@ $^ -ldl



clean:
	-rm -f $(BIN) $(OBJS) *~
