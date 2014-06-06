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

#ifndef _HEXAGON_BOARD_HPP_
#define _HEXAGON_BOARD_HPP_

#include <algorithm>
#include <map>
#include <vector>
#include <cassert>
#include <cstdlib>
#include <fea/rendering/renderer2d.hpp>
#include <fea/rendering/quad.hpp>
#include <cyvmath/hexagon.hpp>

template<int l>
class HexagonBoard
{
	public:
		typedef typename cyvmath::hexagon<l> Hexagon;
		typedef typename Hexagon::Coordinate Coordinate;
		typedef typename std::map<Coordinate, fea::Quad*> TileMap;
		typedef std::vector<fea::Quad*> TileVec;

	private:
		// non-copyable
		HexagonBoard(const HexagonBoard&) = delete;
		const HexagonBoard& operator= (const HexagonBoard&) = delete;

		fea::Renderer2D& _renderer;

		bool _upsideDown;

		// Changing one of these uvec2 variables
		// has very weird effects on the vertical
		// positioning of the board...
		// altough it seems to simply work like this.
		glm::uvec2 _size;
		glm::uvec2 _position;
		glm::vec2 _tileSize;

		TileMap _tileMap;
		TileVec _tileVec;

	public:
		glm::vec2 getTilePosition(Coordinate c)
		{
			glm::vec2 ret;

			if(!_upsideDown) // 'normal' orientation first
			{
				ret.x = _position.x // padding
					// normal horizontal position offset
					+ _tileSize.x * c.x()
					// additional horizontal offset due to non-orthogonal y axis
					+ (_tileSize.x / 2.0f) * (c.y() - (l - 1));

				ret.y = _position.y // padding
					// inverted normal vertical offset
					// because coordinate y = 0 should be on the bottom
					// but Feather Kit has y = 0 on the top
					+ (_size.y - (_tileSize.y * c.y()))
					// to get correct origin point with inverted offsets
					- _tileSize.y;
			}
			else // upsideDown
			{
				ret.x = _position.x // padding
					// inverted normal horizontal position offset
					+ (_size.x - (_tileSize.x * c.x()))
					// to get correct origin point with inverted offsets
					- _tileSize.x
					// additional horizontal offset due to non-orthogonal y axis
					- (_tileSize.x / 2.0f) * (c.y() - (l - 1));

				ret.y = _position.y // padding
					// twice-inverted -> normal vertical offset
					+ _tileSize.y * c.y();
			}

			return ret;
		}

		std::unique_ptr<Coordinate> getCoordinate(glm::ivec2 tilePosition)
		{
			// remove padding - we're just altering a temporary variable
			tilePosition -= glm::ivec2(_position.x, _position.y);

			float x, y;

			if(!_upsideDown) // 'normal' orientation first
			{
				// y = (maximal y) - ('inverted' coord y)
				y = (l * 2 - 1)
					- tilePosition.y / _tileSize.y;

				x = (
						tilePosition.x
						+ ((l - static_cast<int>(y)) * _tileSize.x / 2.0f)
						- _tileSize.x / 2.0f
					)
					/ _tileSize.x;
			}
			else // upsideDown
			{
				y = tilePosition.y / _tileSize.y;

				x = (l * 2 - 1)
					- ((
							tilePosition.x
							- ((l - static_cast<int>(y)) * _tileSize.x / 2.0f)
							+ _tileSize.x / 2.0f
						)
						/ _tileSize.x
					);
			}

			if(x < 0.0f || y < 0.0f)
				return nullptr;

			return std::unique_ptr<Coordinate>(
					Coordinate::create(static_cast<int>(x), static_cast<int>(y))
				);
		}

		fea::Quad* getTileAt(Coordinate c)
		{
			typename TileMap::iterator it = _tileMap.find(c);
			if(it == _tileMap.end())
				return nullptr;

			return it->second;
		}

		fea::Color getTileColor(Coordinate c, bool setup)
		{
			static fea::Color tileColors[3] = {
					{125,  81,  55},
					{167, 108,  73},
					{191, 140, 109}
				};
			static fea::Color tileColorsDark[3] = {
					{ 50,  32,  22},
					{ 67,  43,  29},
					{ 76,  56,  44}
				};

			int8_t index = (((c.x() - c.y()) % 3) + 3) % 3;

			if(setup && ((!_upsideDown && c.y() >= (l - 1)) ||
			              (_upsideDown && c.y() <= (l - 1))))
				return tileColorsDark[index];
			else
				return tileColors[index];
		}

		HexagonBoard(fea::Renderer2D& renderer, bool upsideDown)
			: _renderer(renderer)
			, _upsideDown(upsideDown)
		{
			const glm::uvec2 windowSize = renderer.getViewport().getSize();

			unsigned padding = std::min(windowSize.x, windowSize.y) / 20;

			_size = {windowSize.x - padding * 2, windowSize.y - padding * 2};

			if(_size.x / _size.y < 4.0f / 3.0f) // wider than 4:3
			{
				_tileSize.y = _size.y / static_cast<float>(l * 2 - 1);
				_tileSize.x = _tileSize.y / 3.0f * 4.0f;

				_size.x = _tileSize.x * (l * 2 - 1);

				_position = {(windowSize.x - _size.x) / 2.0f, padding};
			}
			else
			{
				_tileSize.x = _size.x / static_cast<float>(l * 2 - 1);
				_tileSize.y = _tileSize.x / 4.0f * 3.0f;

				_size.y = _tileSize.x * (l * 2 - 1);

				_position = {padding, (windowSize.y - _size.y) / 2.0f};
			}

			for(Coordinate c : Hexagon::getAllCoordinates())
			{
				fea::Quad* quad = new fea::Quad(_tileSize);

				quad->setPosition(getTilePosition(c));
				quad->setColor(getTileColor(c, true));

				// add the tile to the map and vector
				_tileVec.push_back(quad);
				std::pair<typename TileMap::iterator, bool> res = _tileMap.insert({c, quad});

				assert(res.second); // assert the insertion was successful
			}
		}

		~HexagonBoard()
		{
			for(fea::Quad* it : _tileVec)
				delete it;
		}

		const glm::vec2& getTileSize() const
		{
			return _tileSize;
		}

		void updateTileColors(int8_t fromRow, int8_t toRow, bool setup = false)
		{
			assert(fromRow <= toRow);
			for(auto it : _tileMap)
			{
				if(it.first.y() >= fromRow && it.first.y() <= toRow)
					it.second->setColor(getTileColor(it.first, setup));
			}
		}

		void tick()
		{
			for(const fea::Quad* it : _tileVec)
				_renderer.queue(*it);
		}
};

#endif // _HEXAGON_BOARD_HPP_
