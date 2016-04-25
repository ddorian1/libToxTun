noinst_PROGRAMS = test

test_SOURCES = \
	../src/Data.hpp \
	Data.cpp \
	test.cpp \
	tox_dummy.c

test_CPPFLAGS = -std=c++11 $(AM_CXXFLAGS)
test_LDFLAGS = @BOOST_UNIT_TEST_FRAMEWORK_LIB@ @BOOST_ASIO_LIB@ ../src/.libs/libtoxtun.a $(AM_LDFLAGS)
