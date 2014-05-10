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
#include <featherkit/rendering/renderer2d.hpp>
#include <featherkit/rendering/quad.hpp>
#include "hexagon.hpp"

// Should probably be a template as cyvmath::hexagon but that would
// be quite complex and for now we only need hexagon<6> anyway.
class HexagonalBoard : public Board
{
	public:
		typedef cyvmath::hexagon<6> Hexagon;
		typedef Hexagon::Coordinate Coordinate;
		typedef std::unordered_map<Coordinate, fea::Quad*, std::hash<int>> tileMap;
		typedef std::vector<fea::Quad*> tileVec;

	private:
		// non-copyable
		HexagonalBoard(const HexagonalBoard&) = delete;
		const HexagonalBoard& operator= (const HexagonalBoard&) = delete;

		tileMap _tileMap;
		tileVec _tileVec;

	public:
		HexagonalBoard(fea::Renderer2D&);
		~HexagonalBoard();

		void setup() override;
		void tick() override;
};

#endif // _HEXAGONAL_BOARD_HPP_
