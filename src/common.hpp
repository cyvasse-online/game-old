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

#ifndef _COMMON_HPP_
#define _COMMON_HPP_

#include <featherkit/rendering/quad.hpp>
#include <featherkit/ui/event.hpp>

bool mouseOver(fea::Quad& quad, fea::Event event)
{
	assert(event.type == fea::Event::MOUSEBUTTONPRESSED ||
	       event.type == fea::Event::MOUSEBUTTONRELEASED ||
	       event.type == fea::Event::MOUSEMOVED);

	int mX, mY; // mouse x and y coordinates
	if(event.type == fea::Event::MOUSEMOVED)
	{
		mX = event.mouseMove.x;
		mY = event.mouseMove.y;
	}
	else
	{
		mX = event.mouseButton.x;
		mY = event.mouseButton.y;
	}

	glm::vec2 quadPos = quad.getPosition(), quadSize = quad.getSize();

	return (mX >= quadPos.x && mX < quadPos.x + quadSize.x) &&
	       (mY >= quadPos.y && mY < quadPos.y + quadSize.y);
}

#endif // _COMMON_HPP_
