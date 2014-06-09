AUTOMAKE_OPTIONS = subdir-objects

SUBDIRS = cyvmath

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
	src/common.hpp \
	src/cyvasse_app.cpp \
	src/cyvasse_app.hpp \
	src/hexagon_board.hpp \
	src/ingame_state.cpp \
	src/ingame_state.hpp \
	src/main.cpp \
	src/rule_set_base.hpp \
	src/mikelepage/local_player.cpp \
	src/mikelepage/local_player.hpp \
	src/mikelepage/mikelepage_rule_set.cpp \
	src/mikelepage/mikelepage_rule_set.hpp \
	src/mikelepage/rendered_piece.cpp \
	src/mikelepage/rendered_piece.hpp

# The last include directory contains lodepng,
# which loads png files, plus texturemaker.hpp
# which uses lodepng to create fea::Texture objects.
# This functionality will probably be included
# in Feather Kit in the future, so [TODO] this
# folder should be removed then.
cyvasse_js_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/cyvmath/include \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/lodepng

cyvasse_js_CXXFLAGS = \
	$(AM_CXXFLAGS) \
	-O3

cyvasse_js_LDFLAGS = \
	$(AM_LDFLAGS) \
	--preload-file $(top_srcdir)/data@/ \
	-O3

cyvasse_js_LDADD = \
	-lfea-structure \
	-lfea-ui \
	-lfea-rendering \
	-lfea-sdl
