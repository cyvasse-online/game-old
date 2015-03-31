/* Copyright 2014 Jonas Platte
 *
 * This file is part of Cyvasse Online.
 *
 * Cyvasse Online is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * Cyvasse Online is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include "cyvasse_app.hpp"

#include <map>
#include <memory>

#ifdef __EMSCRIPTEN__
	#include <emscripten.h>
#endif

#include "ingame_state.hpp"
#include "rendered_match.hpp"

using namespace std;
using namespace cyvasse;

#include <SDL2/SDL.h>

CyvasseApp::CyvasseApp()
	: m_window(fea::VideoMode(800, 600, 32), "Cyvasse Online")
	, m_renderer(fea::Viewport({800, 600}, {0, 0}, fea::Camera({800.0f / 2.0f, 600.0f / 2.0f})))
{
	SDL_SetMainReady();

	SDL_EventState(SDL_MOUSEWHEEL, SDL_DISABLE);

	SDL_EventState(SDL_TEXTINPUT, SDL_DISABLE);
	SDL_EventState(SDL_KEYDOWN, SDL_DISABLE);
	SDL_EventState(SDL_KEYUP, SDL_DISABLE);

	SDL_EventState(SDL_FINGERMOTION, SDL_DISABLE);
	SDL_EventState(SDL_FINGERDOWN, SDL_DISABLE);
	SDL_EventState(SDL_FINGERUP, SDL_DISABLE);

	m_renderer.setup();

	auto ingameState = make_unique<IngameState>(bind(&fea::Window::pollEvent, &m_window, placeholders::_1), m_renderer);

	#ifdef __EMSCRIPTEN__
	EM_ASM(
		if(typeof(gameMetaData) !== 'object' || gameMetaData === null) {
			throw new Error('No meta data found!');
		}
	);

	auto color   = StrToPlayersColor(emscripten_run_script_string("gameMetaData.color"));
	#else
	// --- hardcoded only until game init code is written ---
	auto color = PlayersColor::WHITE;
	#endif

	m_match = make_unique<RenderedMatch>(*ingameState, m_renderer, color);

	m_stateMachine.addGameState("ingame", move(ingameState));
//#ifdef __EMSCRIPTEN__
	m_stateMachine.setCurrentState("ingame");
//#else
	//m_stateMachine.addGameState("startpage", /* ... */);
	//m_stateMachine.setCurrentState("startpage");
//#endif
}

void CyvasseApp::loop()
{
	// let the state machine run the current game state
	m_stateMachine.run();

	// display whatever the current game state rendered
	m_window.swapBuffers();

	// exit the program when the state machine is finished
	if(m_stateMachine.isFinished())
		quit();
}
