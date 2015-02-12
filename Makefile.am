AUTOMAKE_OPTIONS = subdir-objects

SUBDIRS = cyvasse-common .

game_sources = \
	lodepng/lodepng.cpp \
	src/cyvasse_app.cpp \
	src/cyvasse_ws_client.cpp \
	src/ingame_state.cpp \
	src/main.cpp \
	src/mikelepage/local_player.cpp \
	src/mikelepage/remote_player.cpp \
	src/mikelepage/rendered_fortress.cpp \
	src/mikelepage/rendered_match.cpp \
	src/mikelepage/rendered_piece.cpp \
	src/mikelepage/rendered_terrain.cpp

# The last include directory contains lodepng,
# which loads png files, plus texturemaker.hpp
# which uses lodepng to create fea::Texture objects.
# This functionality will probably be included
# in Feather Kit in the future, so [TODO] this
# folder should be removed then.
game_cppflags = \
	-I$(top_srcdir)/cyvasse-common/include \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/lodepng

game_cxxflags = \
	$(FEA_STRUCTURE_CFLAGS) \
	$(FEA_UI_CFLAGS) \
	$(FEA_RENDERING_CFLAGS) \
	$(FEA_SDL_CFLAGS) \
	$(JSONCPP_CFLAGS)

game_ldadd = \
	$(top_builddir)/cyvasse-common/libcyvmath.a \
	$(top_builddir)/cyvasse-common/libcyvws.a \
	$(FEA_STRUCTURE_LIBS) \
	$(FEA_UI_LIBS) \
	$(FEA_RENDERING_LIBS) \
	$(FEA_SDL_LIBS) \
	$(JSONCPP_LIBS)

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
	-s DISABLE_EXCEPTION_CATCHING=0 \
	--embed-file $(top_builddir)/res@/res

cyvasse_js_LDADD = $(game_ldadd)

endif
