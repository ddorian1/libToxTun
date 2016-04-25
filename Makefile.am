include aminclude.am

if COND_TEST
MAYBE_TEST = test
else
MAYBE_TEST = 
endif

SUBDIRS = src $(MAYBE_TEST)

ACLOCAL_AMFLAGS = -I m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libtoxtun.pc

EXTRA_DIST = autogen.sh Doxyfile libtoxtun.pc.in
