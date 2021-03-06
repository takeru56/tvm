CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS: .c=.o)

tvm: main.c vm.c vm.h
				$(CC) -o tvm $(OBJS) $(LDFLAGS)

test: tvm
				sh test.sh

clean:
				rm -f tvm *.o *~ tmp*

.PHONY: test clean
