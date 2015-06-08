lib_LTLIBRARIES = libtoxtun.la

libtoxtun_ladir = $(includedir)/toxtun

libtoxtun_la_HEADERS = \
	ToxTun.hpp \
	ToxTunC.h \
	Tun.hpp \
	TunUnix.hpp \
	TunWin.hpp

libtoxtun_la_SOURCES = \
	$(libtoxtun_la_HEADERS) \
	Data.cpp \
	Data.hpp \
	Error.hpp \
	Logger.hpp \
	ToxTun.cpp \
	ToxTunC.cpp \
	Tun.cpp \
	TunUnix.cpp \
	TunWin.cpp

libtoxtun_la_CPPFLAGS = -std=c++11 $(AM_CXXFLAGS)
libtoxtun_la_LDFLAGS = $(AM_LDFLAGS)
