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

#include <fea/ui/sdlwindowbackend.hpp>
#include <fea/ui/sdlinputbackend.hpp>

#include <cyvmath/rule_sets.hpp>
#include "ingame_state.hpp"

#ifdef __EMSCRIPTEN__
	#include <emscripten.h>
#endif

#include "mikelepage/rendered_match.hpp"

using namespace cyvmath;

void CyvasseApp::setup(const std::vector<std::string>& /* args */)
{
	static map<RuleSet, function<unique_ptr<Match>(IngameState&, fea::Renderer2D&, PlayersColor)>>
		createMatch {{
			RuleSet::MIKELEPAGE, [](IngameState& st, fea::Renderer2D& r, PlayersColor c)
				{ return unique_ptr<Match>(new ::mikelepage::RenderedMatch(st, r, c)); }
		}};

	m_window.create(fea::VideoMode(800, 600, 32), "Cyvasse");
	m_window.setFramerateLimit(60);

	m_renderer.setup();

	auto ingameState = make_unique<IngameState>(m_input, m_renderer);

	#ifdef __EMSCRIPTEN__
	EM_ASM(
		if(typeof(gameMetaData) !== 'object' || gameMetaData === null) {
			throw new Error('No meta data found!');
		}
	);

	auto ruleSet = StrToRuleSet(emscripten_run_script_string("gameMetaData.ruleSet"));
	auto color   = StrToPlayersColor(emscripten_run_script_string("gameMetaData.color"));
	#else
	// --- hardcoded only until game init code is written ---
	auto ruleSet = RuleSet::MIKELEPAGE;
	auto color = PlayersColor::WHITE;
	#endif

	m_match = createMatch[ruleSet](*ingameState, m_renderer, color);

	m_stateMachine.addGameState("ingame", std::move(ingameState));
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

void CyvasseApp::destroy()
{
	m_window.close();
}

CyvasseApp::CyvasseApp()
	: m_window(new fea::SDLWindowBackend())
	, m_input(new fea::SDLInputBackend())
	, m_renderer(fea::Viewport({800, 600}, {0, 0}, fea::Camera({800.0f / 2.0f, 600.0f / 2.0f})))
{
}
