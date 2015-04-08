AC_INIT([cyvasse-game],[1.0rc1],[mail@jonasplatte.de])
AC_CONFIG_AUX_DIR([.])
AC_CONFIG_MACRO_DIRS([m4])
AM_INIT_AUTOMAKE([foreign])
AM_SILENT_RULES([yes])

AC_PROG_CXX
AC_LANG(C++)
AX_CHECK_COMPILE_FLAG([-std=c++14], [CXXFLAGS="$CXXFLAGS -std=c++14"],
	AX_CHECK_COMPILE_FLAG([-std=c++1y], [CXXFLAGS="$CXXFLAGS -std=c++1y"],
		AC_MSG_ERROR([Compiler doesn't support C++14])
	)
)
AX_CHECK_COMPILE_FLAG([-Wall], [CXXFLAGS="$CXXFLAGS -Wall"])
AX_CHECK_COMPILE_FLAG([-Wextra], [CXXFLAGS="$CXXFLAGS -Wextra"])
AX_CHECK_COMPILE_FLAG([-pedantic], [CXXFLAGS="$CXXFLAGS -pedantic"])

AC_CHECK_FUNCS([sigaction])

PKG_CHECK_MODULES([FEA_STRUCTURE], [fea-structure])
PKG_CHECK_MODULES([FEA_UI], [fea-ui])
PKG_CHECK_MODULES([FEA_RENDERING], [fea-rendering])
PKG_CHECK_MODULES([JSONCPP], [jsoncpp])
PKG_CHECK_MODULES([SDL2], [sdl2])

## This is probably not the best way to check whether emconfigure is used
AM_CONDITIONAL([USING_EMSCRIPTEN], [test -n "$EMMAKEN_JUST_CONFIGURE"])

AC_CONFIG_SUBDIRS([cyvasse-common])
AC_CONFIG_LINKS([res:$srcdir/data])
AC_CONFIG_FILES([
	Makefile
])
AC_OUTPUT
