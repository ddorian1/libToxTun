lib_LTLIBRARIES = libtoxtun.la

libtoxtun_ladir = $(includedir)/toxtun

libtoxtun_la_HEADERS = \
	ToxTun.hpp \
	ToxTunC.h

libtoxtun_la_SOURCES = \
	$(libtoxtun_la_HEADERS) \
	Connection.cpp \
	Connection.hpp \
	Data.cpp \
	Data.hpp \
	Logger.hpp \
	ToxTun.cpp \
	ToxTunC.cpp \
	ToxTunCore.cpp \
	ToxTunCore.hpp \
	ToxTunError.cpp \
	Tun.cpp \
	Tun.hpp \
	TunUnix.cpp \
	TunUnix.hpp \
	TunWin.cpp \
	TunWin.hpp

libtoxtun_la_CPPFLAGS = -std=c++11 $(AM_CXXFLAGS)
libtoxtun_la_LDFLAGS = $(AM_LDFLAGS) -version-info 0:0:0
