# 
# Makefile
# Created: Sun Feb 25 20:36:09 2001 by tek@wiw.org
# Revised: Sun Feb 25 20:36:09 2001 (pending)
# Copyright 2001 Julian E. C. Squires (tek@wiw.org)
# This program comes with ABSOLUTELY NO WARRANTY.
# $Id$
# 
#

CFLAGS=-Wall -pedantic -g
NOOSEOBJS=main.o nntp.o newsrc.o rl.o

noose: $(NOOSEOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

install:
	install -s -m 755 noose /usr/local/bin

clean:
	rm -f *~ *.o

realclean: clean
	rm -f noose

# EOF Makefile#
