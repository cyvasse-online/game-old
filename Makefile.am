AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = -I m4

## probably not the best way to do this, but until we
## create a native program from this source it's okay
CXX = emcc

AM_CPPFLAGS = -I$(EMSCRIPTEN_PREFIX)/include
AM_LDFLAGS  = -L$(EMSCRIPTEN_PREFIX)/lib

AM_CXXFLAGS = -std=c++11

bin_PROGRAMS = cyvasse.js

cyvasse_js_SOURCES = \
	src/cyvasse_app.cpp \
	src/cyvasse_app.hpp \
	src/main.cpp

cyvasse_js_LDADD = -lfeatherkit-structure -lfeatherkit-rendering -lfeatherkit-sdl
