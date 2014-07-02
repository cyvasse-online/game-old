AUTOMAKE_OPTIONS = subdir-objects

SUBDIRS = cyvmath .

game_sources = \
	lodepng/lodepng.cpp \
	lodepng/lodepng.h \
	lodepng/texturemaker.hpp \
	src/common.hpp \
	src/cyvasse_app.cpp \
	src/cyvasse_app.hpp \
	src/cyvasse_ws_client.cpp \
	src/cyvasse_ws_client.hpp \
	src/hexagon_board.hpp \
	src/ingame_state.cpp \
	src/ingame_state.hpp \
	src/main.cpp \
	src/mikelepage/local_player.hpp \
	src/mikelepage/mikelepage_rule_set.cpp \
	src/mikelepage/mikelepage_rule_set.hpp \
	src/mikelepage/remote_player.cpp \
	src/mikelepage/remote_player.hpp \
	src/mikelepage/rendered_piece.cpp \
	src/mikelepage/rendered_piece.hpp

# The last include directory contains lodepng,
# which loads png files, plus texturemaker.hpp
# which uses lodepng to create fea::Texture objects.
# This functionality will probably be included
# in Feather Kit in the future, so [TODO] this
# folder should be removed then.
game_cppflags = \
	-I$(top_srcdir)/cyvmath/include \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/lodepng

game_cxxflags = \
	-std=c++11

game_ldadd = \
	$(top_builddir)/cyvmath/libcyvmath.a \
	-lfea-structure \
	-lfea-ui \
	-lfea-rendering \
	-lfea-sdl \
	-ljsoncpp

if !USING_EMSCRIPTEN # native

bin_PROGRAMS = cyvasse-game

cyvasse_game_SOURCES = $(game_sources)

cyvasse_game_CPPFLAGS = \
	$(game_cppflags)

cyvasse_game_CXXFLAGS = \
	$(game_cxxflags) \
	-pthread

cyvasse_game_LDFLAGS = \
	-pthread

cyvasse_game_LDADD = \
	$(game_ldadd) \
	-lboost_system

else USING_EMSCRIPTEN # cross-compiling to js

bin_PROGRAMS = cyvasse.js

cyvasse_js_SOURCES = $(game_sources)

cyvasse_js_CXXFLAGS = \
	$(game_cppflags) \
	$(game_cxxflags) \
	-I$(EMSCRIPTEN_PREFIX)/include \
	-Wno-warn-absolute-paths

cyvasse_js_LDFLAGS = \
	-L$(EMSCRIPTEN_PREFIX)/lib \
	-s EXPORTED_FUNCTIONS="['_main', '_game_handlemessage']" \
	-s FULL_ES2=1 \
	--preload-file $(top_srcdir)/data@/

cyvasse_js_LDADD = $(game_ldadd)

endif
