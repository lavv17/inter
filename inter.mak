
CFLAGS=-ml -v -DMSDOS
CC=bcc
AR=tlib 
OBJS=inter.obj init.obj check.obj idos.obj ow.obj timer.obj
TLIBCMD=+inter.obj +init.obj +check.obj +idos.obj +ow.obj +timer.obj

.c.obj:
	$(CC) -c $(CFLAGS) $<

inter.lib:	$(OBJS)
	del inter.lib
	tlib inter.lib /c $(TLIBCMD)

inter.obj:	inter.c inter.h
init.obj:	init.c inter.h
check.obj:	check.c check.h
idos.obj:	idos.c inter.h

test.exe:	inter.lib test.c
	$(CC) $(CFLAGS) test.c inter.lib
