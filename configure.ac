AC_INIT([cyvasse-game],[0.1],[mail@jonasplatte.de])
AC_CONFIG_AUX_DIR([.])
AM_INIT_AUTOMAKE([1.11 foreign])
AM_SILENT_RULES([yes])
AC_PROG_CXX

AC_CHECK_FUNCS([sigaction])

## This is probably not the best way to check whether emconfigure is used
AM_CONDITIONAL([USING_EMSCRIPTEN], [test -n "$EMSCRIPTEN"])

AC_ARG_VAR([EMSCRIPTEN_PREFIX], [prefix in which to search for library bitcode for emscripten])

AC_CONFIG_SUBDIRS([cyvmath])
AC_CONFIG_FILES([
	Makefile
])
AC_OUTPUT
