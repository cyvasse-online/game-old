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

#include <unordered_map>
#include <vector>
#include <cassert>
#include <cstdlib>
#include <featherkit/rendering/renderer2d.hpp>
#include <featherkit/rendering/quad.hpp>
#include "hexagon.hpp"

template<int l>
class HexagonalBoard
{
	public:
		typedef typename cyvmath::hexagon<l> Hexagon;
		typedef typename Hexagon::Coordinate Coordinate;
		typedef typename std::unordered_map<Coordinate, fea::Quad*, std::hash<int>> tileMap;
		typedef std::vector<fea::Quad*> tileVec;

	private:
		// non-copyable
		HexagonalBoard(const HexagonalBoard&) = delete;
		const HexagonalBoard& operator= (const HexagonalBoard&) = delete;

		fea::Renderer2D& _renderer;

		glm::vec2 _size;
		glm::vec2 _realSize;
		glm::vec2 _position;
		glm::vec2 _tileSize;

		tileMap _tileMap;
		tileVec _tileVec;

		static int8_t getColorIndex(Coordinate c)
		{
			return (((c.x() - c.y()) % 3) + 3) % 3;
		}

	public:
		glm::vec2 getTilePosition(Coordinate c)
		{
			// TODO: document
			return {_position.x + _tileSize.x * c.x() + (_tileSize.x / 2) * (c.y() - (Hexagon::edgeLength - 1)),
			        _position.y - _tileSize.y + (_realSize.y - (_tileSize.y * c.y()))};
		}

		HexagonalBoard(fea::Renderer2D& renderer, glm::vec2 size, glm::vec2 offset)
			: _renderer(renderer)
			, _size(size)
		{
			// the tiles map never changes so it is set up here instead of in setup()
			fea::Color tileColors[3] = {
					{1.0f, 1.0f, 1.0f},
					{0.8f, 0.8f, 0.8f},
					{0.6f, 0.6f, 0.6f}
				};
			fea::Color tileColorsDark[3] = {
					{0.4f, 0.4f, 0.4f},
					{0.3f, 0.3f, 0.3f},
					{0.2f, 0.2f, 0.2f}
				};

			float tileWidth = _size.x / static_cast<float>(l * 2 - 1);
			_tileSize = {tileWidth, tileWidth / 3 * 2};

			_realSize = {_size.x, (l * 2 - 1) * _tileSize.y};

			_position = {
					offset.x,
					offset.y + (_size.y - _realSize.y) / 2
				};

			for(Coordinate c : Hexagon::getAllCoordinates())
			{
				fea::Quad* quad = new fea::Quad(_tileSize);

				quad->setPosition(getTilePosition(c));

				// may be no permanent solution
				if(c.y() >= (l - 1))
					quad->setColor(tileColorsDark[getColorIndex(c)]);
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

		const glm::vec2& getTileSize() const
		{
			return _tileSize;
		}

		void tick()
		{
			for(const fea::Quad* it : _tileVec)
			{
				_renderer.queue(*it);
			}
		}
};

#endif // _HEXAGONAL_BOARD_HPP_
