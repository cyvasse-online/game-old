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

#include "mikelepage/rendered_match.hpp"

using namespace cyvmath;

IngameState::IngameState(fea::InputHandler& inputHandler, fea::Renderer2D& renderer)
	: _input(inputHandler)
	, _renderer(renderer)
	, _background(renderer.getViewport().getSize())
{
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
		// calling all event handler functors unconditionally
		// means they all have to have a callable set because
		// else they will raise an exception.
		switch(event.type)
		{
			case fea::Event::CLOSED:
				return "NONE";
				break;
			case fea::Event::MOUSEMOVED:
				onMouseMoved(event.mouseMove);
				break;
			case fea::Event::MOUSEBUTTONPRESSED:
				onMouseButtonPressed(event.mouseButton);
				break;
			case fea::Event::MOUSEBUTTONRELEASED:
				onMouseButtonReleased(event.mouseButton);
				break;
			case fea::Event::KEYPRESSED:
				onKeyPressed(event.key);
				break;
			case fea::Event::KEYRELEASED:
				onKeyReleased(event.key);
				break;
		}
	}

	// after events were processed
	// * clear the rendered content from the last frame
	_renderer.clear();

	// * queue something to render
	_renderer.queue(_background);
	tick();

	// * render everything
	_renderer.render();

	// keep running the same state
	return std::string();
}
