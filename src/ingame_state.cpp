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

#include "ingame_state.hpp"

IngameState::IngameState(fea::InputHandler& inputHandler, fea::Renderer2D& renderer)
	: _input(inputHandler)
	, _renderer(renderer)
{
}

// ----- begin test code -----

#include "rule_sets/mikelepage_rule_set.hpp"

void IngameState::setup()
{
	_ruleSet = std::unique_ptr<RuleSet>(new MikelepageRuleSet(_renderer, PLAYER_WHITE));
}

// ------ end test code ------

std::string IngameState::run()
{
	fea::Event event;
	while(_input.pollEvent(event))
	{
		// won't happen when compiled to js,
		// may be of interest somewhen later
		//if(event.type == fea::Event::CLOSED)

		// may be of interest somewhen later
		//if(event.type == fea::Event::KEYPRESSED)

		if(event.type == fea::Event::MOUSEBUTTONPRESSED)
			; // do something

		if(event.type == fea::Event::MOUSEBUTTONRELEASED)
			; // do something
	}
	// after events were processed
	// * clear the rendered content from the last frame
	_renderer.clear();

	// * queue something to render
	_ruleSet->tick();

	// * render everything
	_renderer.render();

	// keep running the same state
	return std::string();
}

void IngameState::initMatch(RuleSet& ruleSet)
{
	_ruleSet = std::unique_ptr<RuleSet>(&ruleSet);
}
