AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = -I m4

## probably not the best way to do this, but until we
## create a native program from this source it's okay
CXX = emcc

AM_CPPFLAGS = -I$(EMSCRIPTEN_PREFIX)/include
AM_LDFLAGS  = -L$(EMSCRIPTEN_PREFIX)/lib

# Second option is necessary for Feather Kit to work with emscripten
# Third option disables warnings one gets when EMSCRIPTEN_PREFIX is an absolute path
AM_CXXFLAGS = \
	-std=c++11 \
	-s FULL_ES2=1 \
	-Wno-warn-absolute-paths

bin_PROGRAMS = cyvasse.js

cyvasse_js_SOURCES = \
	src/board.cpp \
	src/board.hpp \
	src/cyvasse_app.cpp \
	src/cyvasse_app.hpp \
	src/ingame_state.cpp \
	src/ingame_state.hpp \
	src/main.cpp \
	src/ruleset.cpp \
	src/ruleset.hpp

cyvasse_js_LDADD = \
	-lfeatherkit-structure \
	-lfeatherkit-ui \
	-lfeatherkit-rendering \
	-lfeatherkit-sdl
