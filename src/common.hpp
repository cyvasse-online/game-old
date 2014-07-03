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

#include <fea/rendering/quad.hpp>
#include <fea/ui/event.hpp>

bool mouseOver(fea::Quad& quad, glm::uvec2 mousePos)
{
	glm::vec2 quadPos = quad.getPosition(), quadSize = quad.getSize();

	return (mousePos.x >= quadPos.x && mousePos.x < quadPos.x + quadSize.x) &&
	       (mousePos.y >= quadPos.y && mousePos.y < quadPos.y + quadSize.y);
}

#endif // _COMMON_HPP_
