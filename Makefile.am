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
	lodepng/lodepng.cpp \
	lodepng/lodepng.h \
	lodepng/texturemaker.hpp \
	src/board.cpp \
	src/board.hpp \
	src/cyvasse_app.cpp \
	src/cyvasse_app.hpp \
	src/ingame_state.cpp \
	src/ingame_state.hpp \
	src/main.cpp \
	src/rule_set.cpp \
	src/rule_set.hpp \
	src/boards/hexagonal_board.hpp \
	src/rule_sets/mikelepage_rule_set.cpp \
	src/rule_sets/mikelepage_rule_set.hpp

# The last include directory contains lodepng,
# which loads png files, plus texturemaker.hpp
# which uses lodepng to create fea::Texture objects.
# This functionality will probably be included
# in Feather Kit in the future, so [TODO] this
# folder should be removed then.
cyvasse_js_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I $(top_srcdir)/src \
	-I $(top_srcdir)/cyvmath \
	-I $(top_srcdir)/lodepng

cyvasse_js_CXXFLAGS = \
	$(AM_CXXFLAGS) \
	-O3

cyvasse_js_LDFLAGS = \
	$(AM_LDFLAGS) \
	--embed-file $(top_srcdir)/data@/ \
	-O3

cyvasse_js_LDADD = \
	-lfeatherkit-structure \
	-lfeatherkit-ui \
	-lfeatherkit-rendering \
	-lfeatherkit-sdl
