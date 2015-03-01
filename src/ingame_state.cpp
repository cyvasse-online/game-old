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

#include "mikelepage/rendered_match.hpp"

using namespace cyvmath;

IngameState::IngameState(std::function<bool(fea::Event&)> pollEvent, fea::Renderer2D& renderer)
	: m_pollEvent(pollEvent)
	, m_renderer(renderer)
	, m_background(renderer.getViewport().getSize())
{
}

void IngameState::setup()
{
	m_background.setColor({255, 255, 255});
}

std::string IngameState::run()
{
	fea::Event event;
	while(m_pollEvent(event))
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
			default: { } // disable compiler warning
		}
	}

	// after events were processed
	// * clear the rendered content from the last frame
	m_renderer.clear();

	// * queue something to render
	m_renderer.queue(m_background);
	tick();

	// * render everything
	m_renderer.render();

	// keep running the same state
	return std::string();
}
