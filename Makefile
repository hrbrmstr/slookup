PROG=slookup

all: $(PROG)

uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
uname_M := $(shell sh -c 'uname -m 2>/dev/null || echo not')
uname_O := $(shell sh -c 'uname -o 2>/dev/null || echo not')
uname_R := $(shell sh -c 'uname -r 2>/dev/null || echo not')
uname_P := $(shell sh -c 'uname -p 2>/dev/null || echo not')
uname_V := $(shell sh -c 'uname -v 2>/dev/null || echo not')

CFLAGS = -Wall -O3 
LDFLAGS = -lresolv
INSTALLDIR = /usr/local/bin

ifdef CXX
CC = $(CXX)
LD = $(CXX)
else 
CC = g++
LD = g++
ifeq ($(uname_S),Darwin)
  CC = clang++
  LD = clang++
endif
endif

.cpp.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o *~ *.bak core

distclean: clean
	rm -f $(PROG)

install: all
	install -s -p slookup /usr/local/bin

BITS = slookup.o

$(PROG): $(BITS)
	$(LD) -o $(PROG) $(BITS) $(LDFLAGS) 

slookup.o:	slookup.cpp

test: $(BITS)
	@mkdir -p bin
	@$(LD) -o bin/$(PROG) $(BITS) $(LDFLAGS)
	@./test.sh
