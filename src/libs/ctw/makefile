OBJS = ctw-settings.o ctwencdec.o ctwtree.o ctwmath.o ctwlarc.o ctwstep.o ctw-header.o

CC = gcc
CCOPT = -O2 -D_PROTOTYPES -H

PROGS = ctw

all:   $(PROGS)

clean:
	-rm -rf *~
	-rm -rf $(PROGS)
	-rm -rf $(OBJS)

ctw: ctw.c $(OBJS)
	$(CC) $(CCOPT) -o ctw ctw.c $(OBJS) -lm

ctwencdec.o: ctwencdec.c ctw-settings.h ctwencdec.h ctwlarc.h ctwstep.h ctwmath.h ctwtree.h
	$(CC) $(CCOPT) -c ctwencdec.c

ctwtree.o: ctwtree.c ctw-settings.h ctwtree.h ctwmath.h
	$(CC) $(CCOPT) -c ctwtree.c

ctwmath.o:ctwmath.c ctw-settings.h ctwmath-tables.h ctwmath.h
	$(CC) $(CCOPT) -c ctwmath.c

ctwlarc.o: ctwlarc.c ctwlarc-tables.h ctwlarc.h
	$(CC) $(CCOPT) -c ctwlarc.c

ctwstep.o: ctwstep.c ctwstep.h ctwmath.h ctwlarc.h
	$(CC) $(CCOPT) -c ctwstep.c

ctw-settings.o: ctw-settings.c
	$(CC) $(CCOPT) -c ctw-settings.c

ctw-header.o: ctw-header.c ctw-settings.h
	$(CC) $(CCOPT) -c ctw-header.c