.PHONY:		all clean again install GNUmakefile

VERSION =	3.50
prefix =	/pro

ifeq ($(OSTYPE), aix)
CC =		xlc
DEFINES =	-DRISC -DIBM -DNLS
endif

ifeq ($(CC), gcc)
CFLAGS =	-O2 -g -static
DEFINES +=	-D_INCLUDE_POSIX_SOURCE
else
ifeq ($(OSTYPE), aix)
CFLAGS =	-O2 -qmaxmem=-1 -w -qnoroconst -qnoro
else
ifeq ($(OSTYPE), hpux)
CFLAGS =	+O2 +Onolimit -Ae
else
ifeq ($(OSTYPE), osf1)
CFLAGS =	-O2 -std
else
ifeq ($(OSTYPE), linux)
CFLAGS =	-O2 -g -Wall
else
CFLAGS =	-O
endif
endif
endif
endif
endif

IPATH =		-I. -I$(prefix)/local/include -I/usr/local/include
LIBS =		-L$(prefix)/local/lib -L/usr/local/lib
ifeq ($(OSTYPE), hpux)
IPATH +=	-I/usr/include/curses_colr
#LIBS +=	-lxcurses
LIBS +=		-lcur_colr
DEFINES +=	-D_XOPEN_SOURCE_EXTENDED
else
ifeq ($(OSTYPE), aix)
LIBS +=		-ltermcap -lxcurses
else
LIBS +=		-lncurses
endif
endif
LDFLAGS =	#-s 

all:		cshmen

cshmen:		cshmen.c
	$(CC) $(CFLAGS) $(LDFLAGS) $(IPATH) $(DEFINES) -o $@ $< $(LIBS)

again:
	-@$(MAKE) clean
	-@$(MAKE)

clean:
	rm -f core a.out *.a *.o cshmen

install:
	@(rm -f $(prefix)/bin/cshmen 2>/dev/null ||\
	     mv $(prefix)/bin/cshmen $prefix)/bin/cshmen)
	cp cshmen xamen $(prefix)/bin

dist:
	mkdir cshmen-$(VERSION)
	ntar -c -T MANIFEST -f - | ( cd cshmen-$(VERSION) ; tar xf - )
	ntar -c -z -f cshmen-$(VERSION).tgz cshmen-$(VERSION)
	rm -rf cshmen-$(VERSION)

#=============== End Of Makefile ======================================
