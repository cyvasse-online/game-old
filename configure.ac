AC_INIT([cyvasse-game],[0.1],[mail@jonasplatte.de])
AC_CONFIG_AUX_DIR([.])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([1.11 foreign])
AM_SILENT_RULES([yes])
AC_PROG_CXX

AC_CHECK_PROG([emcc])

AC_ARG_VAR([EMSCRIPTEN_PREFIX], [prefix in which to search for library bitcode for emscripten])

LT_INIT

AC_CONFIG_FILES([
	Makefile
])
AC_OUTPUT
