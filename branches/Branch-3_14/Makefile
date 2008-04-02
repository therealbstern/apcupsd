topdir:=.

SUBDIRS=src
include autoconf/targets.mak

configure: autoconf/configure.in autoconf/aclocal.m4
	-rm -f config.cache config.log config.out config.status include/config.h
	autoconf --prepend-include=autoconf autoconf/configure.in > configure
	chmod 755 configure
