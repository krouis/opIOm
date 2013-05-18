bin_PROGRAMS = opiom
opiom_SOURCES = opiom.c functions.c functions.h cmdline.c cmdline.h args.h
opiom_CPPFLAGS = -W -Wall -Werror -D_REENTRANT -D_GNU_SOURCE -pedantic 
AM_CPPFLAGS = -I$(top_srcdir)/lib -I$(top_builddir)/lib
LDDADD = lib/libgnu.a
##opiom_CPPFLAGS = -D_GNU_SOURCE
