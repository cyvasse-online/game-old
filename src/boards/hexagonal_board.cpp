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

HexagonalBoard::HexagonalBoard(fea::Renderer2D& renderer)
	: Board(renderer)
{
	// the tiles map never changes so it is set up here instead of in setup()
	fea::Color tileColors[3] = {
			{1.0f, 1.0f, 1.0f},
			{0.8f, 0.8f, 0.8f},
			{0.6f, 0.6f, 0.6f}
		};

	int8_t colorIndex = 0;
	int8_t lastX = -1;
	for(Coordinate c : Hexagon::getAllCoordinates())
	{
		fea::Quad* quad = new fea::Quad({60, 40});

		// TODO: understand where that 150 comes from, move this
		// functionality to hexagon (with the constants parametrized)
		quad->setPosition({
				60 * c.x() + 30 * c.y() - 150 + 20,
				40 * c.y() + 20
			});

		// finding the right color for the next tile...
		if(lastX < c.x()) // We got to a new "column"
		{
			// TODO: optimize and move to hexagon.hpp
			//if   (c == Coordinate( 0, 5)) colorIndex = 0;
			if     (c == *Coordinate::create( 1, 4)) colorIndex = 1;
			else if(c == *Coordinate::create( 2, 3)) colorIndex = 2;
			else if(c == *Coordinate::create( 3, 2)) colorIndex = 0;
			else if(c == *Coordinate::create( 4, 1)) colorIndex = 1;

			else if(c == *Coordinate::create( 5, 0)) colorIndex = 2;

			else if(c == *Coordinate::create( 6, 0)) colorIndex = 1;
			else if(c == *Coordinate::create( 7, 0)) colorIndex = 0;
			else if(c == *Coordinate::create( 8, 0)) colorIndex = 2;
			else if(c == *Coordinate::create( 9, 0)) colorIndex = 1;
			else if(c == *Coordinate::create(10, 0)) colorIndex = 0;
		}
		else
		{
			colorIndex = (colorIndex + 1) % 3;
		}
		lastX = c.x();

		// set the color
		quad->setColor(tileColors[colorIndex]);

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
