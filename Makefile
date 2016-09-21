PROG=slookup

all: $(PROG)

CC = gcc
LD = gcc
CFLAGS = -Wall -Wstrict-prototypes -O3 # -march=pentium4
LDFLAGS = -lresolv
INSTALLDIR = /usr/local/bin

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o *~ *.bak core

distclean: clean
	rm -f $(PROG)

install: all
	install -s -p slookup /usr/local/bin

BITS = slookup.o

$(PROG): $(BITS)
	$(LD) $(LDFLAGS) -o $(PROG) $(BITS)

slookup.o:	slookup.c
