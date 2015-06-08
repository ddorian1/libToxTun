AC_INIT([libtoxtun],[0.0.1],[mail@jschwab.org])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_SRCDIR([src/ToxTun.hpp])

AM_INIT_AUTOMAKE([1.10 no-define foreign])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs
AC_PROG_CXXCPP
AC_PROG_CXX
AC_PROG_INSTALL

# Use the C++ compiler for the following checks
AC_LANG([C++])

# Checks for header files
AC_HEADER_STDC
AC_CHECK_HEADER([string])
AC_CHECK_HEADER([vector])
AC_CHECK_HEADER([sstream])
AC_CHECK_HEADER([iostream])
AC_CHECK_HEADER([list])
AC_CHECK_HEADER([map])

# Check for library tools
LT_INIT

# Add Doxygen support
DX_PDF_FEATURE(OFF)
DX_PS_FEATURE(OFF)
DX_HTML_FEATURE(ON)
DX_INIT_DOXYGEN($PACKAGE_NAME, Doxyfile, doc)

# Debug
AC_MSG_CHECKING([wether to build with debug information])
AC_ARG_ENABLE([debug],
			  [AS_HELP_STRING([--enable-debug],
							  [enable debug data generation (def=no)])],
              [debugit="$enableval"],
			  [debugit=no])
AC_MSG_RESULT([$debugit])

if test x"$debugit" = x"yes";
then
	AC_DEFINE([DEBUG],[],[Debug Mode])
	AM_CXXFLAGS="$AM_CXXFLAGS -Wall -Werror -Wno-uninitialized"
else
	AC_DEFINE([NDEBUG],[],[No-debug Mode])
fi

# Check OS
AC_CANONICAL_HOST
case $host_os in
	*mingw*)
		AM_LDFLAGS="$AM_LDFLAGS -liphlpapi -ltoxcore -no-undefined"
	;;
esac

# Distribute additional compiler and linker flags
AC_SUBST([AM_CXXFLAGS])
AC_SUBST([AM_LDFLAGS])

# Generate files
AC_CONFIG_FILES([Makefile src/Makefile libtoxtun.pc])
AC_OUTPUT
