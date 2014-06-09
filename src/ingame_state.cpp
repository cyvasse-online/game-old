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

#include <map>
#include <fea/ui/inputbackend.hpp>

#include "mikelepage/mikelepage_rule_set.hpp"

using namespace cyvmath;

IngameState::IngameState(fea::InputHandler& inputHandler, fea::Renderer2D& renderer)
	: _input(inputHandler)
	, _renderer(renderer)
	, _background(renderer.getViewport().getSize())
{
}

void IngameState::initMatch(cyvmath::RuleSet ruleSet, cyvmath::PlayersColor playersColor)
{
	// The world needs more crazy C++11 lambda expressions!
	static std::map<cyvmath::RuleSet, std::function<RuleSetBase*(fea::Renderer2D&, cyvmath::PlayersColor)>>
		createRuleSet{{
				RULESET_MIKELEPAGE,
				[](fea::Renderer2D& r, cyvmath::PlayersColor c)
					{ return new ::mikelepage::MikelepageRuleSet(r, c); }
			}/*, {
				RULESET_WHATEVER,
				[](fea::Renderer2D& r, cyvmath::PlayersColor c)
					{ return new WhateverRuleSet(r, c); }
			}*/
		};

	auto it = createRuleSet.find(ruleSet);
	if(it == createRuleSet.end())
		return;

	_ruleSet = std::unique_ptr<RuleSetBase>(it->second(_renderer, playersColor));
}

void IngameState::setup()
{
	_background.setColor({255, 255, 255});
}

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

		/*if(event.type == fea::Event::MOUSEBUTTONPRESSED ||
		   event.type == fea::Event::MOUSEBUTTONRELEASED ||
		   event.type == fea::Event::MOUSEMOVED)
		{*/
		// at the moment we don't have to find the right
		// part of the applications that has to get the
		// input because there only is one part.
		_ruleSet->processEvent(event);
		/*}*/
	}
	// after events were processed
	// * clear the rendered content from the last frame
	_renderer.clear();

	// * queue something to render
	_renderer.queue(_background);
	_ruleSet->tick();

	// * render everything
	_renderer.render();

	// keep running the same state
	return std::string();
}
