# $Id: Makefile,v 1.1 2004/08/18 19:23:26 kman Exp $

OS=$(shell uname -s)

default: detect

detect:
ifeq ($(OS),Linux)
	$(MAKE) linux
else 
ifeq ($(OS),FreeBSD)
	$(MAKE) freebsd
else 
ifeq ($(OS),NetBSD)
	$(MAKE) netbsd
else
ifeq ($(OS),OpenBSD)
	$(MAKE) openbsd
endif
endif
endif
endif

linux:
	cd src && $(MAKE) netrate-linux
    
freebsd:
	cd src && $(MAKE) netrate-freebsd
    
netbsd:
	cd src && $(MAKE) netrate-netbsd

openbsd:
	cd src && $(MAKE) netrate-openbsd

clean:
	cd src && $(MAKE) clean

install:
	cd src && $(MAKE) install

install-strip:
	cd src && $(MAKE) install-strip

uninstall:
	cd src && $(MAKE) uninstall
