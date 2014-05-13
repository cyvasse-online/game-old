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

#ifndef _HEXAGONAL_BOARD_HPP_
#define _HEXAGONAL_BOARD_HPP_

#include "board.hpp"

#include <unordered_map>
#include <vector>
#include <cassert>
#include <cstdlib>
#include <featherkit/rendering/renderer2d.hpp>
#include <featherkit/rendering/quad.hpp>
#include "hexagon.hpp"

template<int l>
class HexagonalBoard : public Board
{
	public:
		typedef typename cyvmath::hexagon<l> Hexagon;
		typedef typename Hexagon::Coordinate Coordinate;
		typedef typename std::unordered_map<Coordinate, fea::Quad*, std::hash<int>> tileMap;
		typedef std::vector<fea::Quad*> tileVec;

		static glm::vec2 getTilePosition(Coordinate c, glm::vec2 tileSize, glm::vec2 offset)
		{
			return {tileSize.x * c.x() + (tileSize.x / 2) * (c.y() - (Hexagon::edgeLength - 1)) + offset.x,
			        tileSize.y * c.y() + offset.y};
		}
		static int8_t getColorIndex(Coordinate c)
		{
			return (((c.x() - c.y()) % 3) + 3) % 3;
		}

	private:
		// non-copyable
		HexagonalBoard(const HexagonalBoard&) = delete;
		const HexagonalBoard& operator= (const HexagonalBoard&) = delete;

		glm::vec2 _position, _size;

		tileMap _tileMap;
		tileVec _tileVec;

	public:
		HexagonalBoard(fea::Renderer2D& renderer, glm::vec2 position, glm::vec2 size)
			: Board(renderer)
			, _position(position)
			, _size(size)
		{
			// the tiles map never changes so it is set up here instead of in setup()
			fea::Color tileColors[3] = {
					{1.0f, 1.0f, 1.0f},
					{0.8f, 0.8f, 0.8f},
					{0.6f, 0.6f, 0.6f}
				};

			glm::vec2 tileSize = {
					_size.x / static_cast<float>((l * 2) - 1),
					_size.y / static_cast<float>((l * 2) - 1)
				};

			for(Coordinate c : Hexagon::getAllCoordinates())
			{
				fea::Quad* quad = new fea::Quad(tileSize);

				quad->setPosition(getTilePosition(c, tileSize, position));

				// may be no permanent solution
				if(c.y() <= (l - 1))
					quad->setColor({0.4f, 0.4f, 0.4f});
				else
					quad->setColor(tileColors[getColorIndex(c)]);

				// add the tile to the map and vector
				_tileVec.push_back(quad);
				std::pair<typename tileMap::iterator, bool> res = _tileMap.insert({c, quad});

				assert(res.second); // assert the insertion was successful
			}
		}

		~HexagonalBoard()
		{
			for(fea::Quad* it : _tileVec)
			{
				delete it;
			}
		}

		void tick() override
		{
			for(const fea::Quad* it : _tileVec)
			{
				_renderer.queue(*it);
			}
		}
};

#endif // _HEXAGONAL_BOARD_HPP_
