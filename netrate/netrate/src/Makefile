# $Id: Makefile,v 1.1 2004/08/18 19:23:26 kman Exp $

CC=gcc
CFLAGS+=-g
REL=$(shell uname -r | cut -d. -f1)
OS=$(shell uname -s)

default:  netrate-linux

netrate-linux:	netrate.o if_linux.o
	$(CC) $(CFLAGS) -o netrate netrate.o if_linux.o
	
netrate-freebsd: netrate.o if_freebsd.o
	$(CC) $(CFLAGS) -o netrate netrate.o if_freebsd.o -lkvm

netrate-netbsd: netrate.o if_netbsd.o
	$(CC) $(CFLAGS) -o netrate netrate.o if_netbsd.o -lkvm

netrate-openbsd: netrate.o if_openbsd.o
	$(CC) $(CFLAGS) -o netrate netrate.o if_openbsd.o -lkvm

netrate.o: 
	$(CC) $(CFLAGS) -c -DOS_$(OS) netrate.c
	
if_linux.o: 
	$(CC) $(CFLAGS) -c if_linux.c
	
if_freebsd.o: 
	$(CC) $(CFLAGS) -c -DFREEBSD=$(REL) if_freebsd.c

if_netbsd.o:
	$(CC) $(CFLAGS) -c if_netbsd.c

if_openbsd.o:
	$(CC) $(CFLAGS) -c if_openbsd.c

clean:
	rm -f *~ *.o netrate
	
install:
	install -c -s -m 0755 -o 0 -g 0 netrate $(DESTDIR)/usr/bin
	
install-strip:
	strip netrate
	install -c -s -m 0755 -o 0 -g 0 netrate $(DESTDIR)/usr/bin
	
uninstall:
	rm -f $(DESTDIR)/usr/bin/netrate
