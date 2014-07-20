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
#include <fea/ui/sdlwindowbackend.hpp>
#include <fea/ui/sdlinputbackend.hpp>
#include <cyvmath/rule_sets.hpp>
#include <make_unique.hpp>
#include "ingame_state.hpp"

#ifdef EMSCRIPTEN
	#include <emscripten.h>
#endif

#include "mikelepage/rendered_match.hpp"

using namespace cyvmath;

void CyvasseApp::setup(const std::vector<std::string>& /* args */)
{
	static std::map<RuleSet, std::function<std::unique_ptr<Match>(IngameState&, fea::Renderer2D&, PlayersColor)>>
		createMatch {{
			RULE_SET_MIKELEPAGE, [](IngameState& st, fea::Renderer2D& r, PlayersColor c)
				{ return std::unique_ptr<Match>(new ::mikelepage::RenderedMatch(st, r, c)); }
		}};

	_window.create(fea::VideoMode(800, 600, 32), "Cyvasse");
	_window.setFramerateLimit(60);

	_renderer.setup();

	auto ingameState = make_unique<IngameState>(_input, _renderer);

	#ifdef EMSCRIPTEN
	EM_ASM(
		if(typeof(Module.gameMetaData) !== 'object' || Module.gameMetaData === null) {
			throw new Error('No meta data found!');
		}
	);

	auto ruleSet = StrToRuleSet(emscripten_run_script_string("Module.gameMetaData.ruleSet"));
	auto color   = StrToPlayersColor(emscripten_run_script_string("Module.gameMetaData.color"));
	#else
	// --- hardcoded only until game init code is written ---
	auto ruleSet = RULE_SET_MIKELEPAGE;
	auto color = PLAYER_WHITE;
	#endif

	_ruleSet = createMatch[ruleSet](*ingameState, _renderer, color);

	_stateMachine.addGameState("ingame", std::move(ingameState));
//#ifdef EMSCRIPTEN
	_stateMachine.setCurrentState("ingame");
//#else
	//_stateMachine.addGameState("startpage", /* ... */);
	//_stateMachine.setCurrentState("startpage");
//#endif
}

void CyvasseApp::loop()
{
	// let the state machine run the current game state
	_stateMachine.run();

	// display whatever the current game state rendered
	_window.swapBuffers();

	// exit the program when the state machine is finished
	if(_stateMachine.isFinished())
		quit();
}

void CyvasseApp::destroy()
{
	_window.close();
}

CyvasseApp::CyvasseApp()
	: _window(new fea::SDLWindowBackend())
	, _input(new fea::SDLInputBackend())
	, _renderer(fea::Viewport({800, 600}, {0, 0}, fea::Camera({800.0f / 2.0f, 600.0f / 2.0f})))
{
}
