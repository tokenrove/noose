# 
# Makefile
# Created: Sun Feb 25 20:36:09 2001 by tek@wiw.org
# Revised: Sun Feb 25 20:36:09 2001 (pending)
# Copyright 2001 Julian E. C. Squires (tek@wiw.org)
# This program comes with ABSOLUTELY NO WARRANTY.
# $Id$
# 
#

INSTALL_TOP=/usr/local
INSTALL_BIN=$(INSTALL_TOP)/bin
INSTALL_MAN=$(INSTALL_TOP)/share/man/man1
CFLAGS=-Wall -pedantic
NOOSEOBJS=main.o nntp.o newsrc.o rl.o

noose: $(NOOSEOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

install:
	install -s -m 755 noose $(INSTALL_BIN)
	install -m 644 noose.1 $(INSTALL_MAN)

clean:
	rm -f *~ *.o

realclean: clean
	rm -f noose

# EOF Makefile#
