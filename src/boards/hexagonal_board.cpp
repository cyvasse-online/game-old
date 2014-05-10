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

#include "hexagonal_board.hpp"

#include <cassert>
#include <cstdlib>

glm::vec2 HexagonalBoard::getTilePosition(Coordinate c, glm::vec2 tileSize, int xOffset, int yOffset)
{
	return {tileSize.x * c.x() + (tileSize.x / 2) * (c.y() - (Hexagon::edgeLength - 1)) + xOffset,
	        tileSize.y * c.y() + yOffset};
}

int8_t HexagonalBoard::getColorIndex(Coordinate c)
{
	return (((c.x() - c.y()) % 3) + 3) % 3;
}

HexagonalBoard::HexagonalBoard(fea::Renderer2D& renderer)
	: Board(renderer)
{
	// the tiles map never changes so it is set up here instead of in setup()
	fea::Color tileColors[3] = {
			{1.0f, 1.0f, 1.0f},
			{0.8f, 0.8f, 0.8f},
			{0.6f, 0.6f, 0.6f}
		};

	for(Coordinate c : Hexagon::getAllCoordinates())
	{
		fea::Quad* quad = new fea::Quad({60, 40});

		quad->setPosition(getTilePosition(c, {60, 40}, 40, 40));
		quad->setColor(tileColors[getColorIndex(c)]);

		// add the tile to the map and vector
		_tileVec.push_back(quad);
		std::pair<tileMap::iterator, bool> res = _tileMap.insert({c, quad});

		assert(res.second); // assert the insertion was successful
	}
}

HexagonalBoard::~HexagonalBoard()
{
	for(fea::Quad* it : _tileVec)
	{
		delete it;
	}
}

void HexagonalBoard::tick()
{
	for(const fea::Quad* it : _tileVec)
	{
		_renderer.queue(*it);
	}
}
