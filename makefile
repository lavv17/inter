
CC=gcc
CFLAGS=-O -g -Wall
LIBS=-lcurses -lmalloc
OBJS=inter.o init.o icurses.o check.o ow.o timer.o

libinter.a:	$(OBJS)
	rm -f $@
	ar q $@ $(OBJS)

inter.o:	inter.c inter.h
icurses.o:	icurses.c inter.h
init.o:		init.c inter.h
iscocons.o:	iscocons.c inter.h
check.o:	check.c check.h
ow.o:		ow.c ow.h

clean:
	rm -f *.o *.obj *.exe test libinter.a

test:    test.c libinter.a
	cc test.c -o test libinter.a $(LIBS)
