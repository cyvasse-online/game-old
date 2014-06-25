AC_INIT([cyvasse-game],[0.1],[mail@jonasplatte.de])
AC_CONFIG_AUX_DIR([.])
AC_CONFIG_MACRO_DIRS([m4])
AM_INIT_AUTOMAKE([1.11 foreign])
AM_SILENT_RULES([yes])
AC_PROG_CXX
AC_LANG(C++)
AX_CXX_COMPILE_STDCXX_11(noext, mandatory)

AC_CHECK_FUNCS([sigaction])

PKG_CHECK_MODULES([FEA_STRUCTURE], [fea-structure])
PKG_CHECK_MODULES([FEA_UI], [fea-ui])
PKG_CHECK_MODULES([FEA_RENDERING], [fea-rendering])
PKG_CHECK_MODULES([FEA_SDL], [fea-sdl])
PKG_CHECK_MODULES([JSONCPP], [jsoncpp])

## This is probably not the best way to check whether emconfigure is used
AM_CONDITIONAL([USING_EMSCRIPTEN], [test -n "$EMMAKEN_JUST_CONFIGURE"])

AC_ARG_VAR([EMSCRIPTEN_PREFIX], [prefix in which to search for library bitcode for emscripten])

AC_CONFIG_SUBDIRS([cyvasse-common])
AC_CONFIG_FILES([
	Makefile
])
AC_OUTPUT
