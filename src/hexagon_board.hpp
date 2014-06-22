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
		typedef typename cyvmath::Hexagon<l> Hexagon;
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
		HexagonBoard(fea::Renderer2D&, cyvmath::PlayersColor);
		~HexagonBoard();

		glm::uvec2 getSize();
		glm::uvec2 getPosition();

		glm::vec2 getTilePosition(Coordinate);
		glm::vec2 getTilePosition(const dc::unique_ptr<Coordinate>& c)
		{
			assert(c);
			return getTilePosition(*c);
		}

		const glm::vec2& getTileSize() const;

		fea::Color getTileColor(Coordinate, bool setup);
		fea::Color getTileColor(const dc::unique_ptr<Coordinate>& c, bool setup)
		{
			assert(c);
			return getTileColor(*c, setup);
		}

		std::unique_ptr<Coordinate> getCoordinate(glm::ivec2 tilePosition);

		fea::Quad* getTileAt(Coordinate);
		fea::Quad* getTileAt(const dc::unique_ptr<Coordinate>& c)
		{
			assert(c);
			return getTileAt(*c);
		}

		void updateTileColors(int8_t fromRow, int8_t toRow, bool setup = false);

		void tick();
};

#include "hexagon_board.inl"

#endif // _HEXAGON_BOARD_HPP_
