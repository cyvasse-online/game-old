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

#include "rendered_fortress.hpp"

#include "hexagon_board.hpp"

namespace mikelepage
{
	RenderedFortress::RenderedFortress(PlayersColor color, Coordinate coord, HexagonBoard<6>& board)
		: Fortress(color, coord)
		, _board(board)
		, _quad(board.getTileSize())
	{
		static const std::map<PlayersColor, fea::Color> colors = {
			{PlayersColor::WHITE, {255, 255, 255, 127}},
			{PlayersColor::BLACK, {0, 0, 0, 127}}
		};

		assert(color != PlayersColor::UNDEFINED);

		_quad.setColor(colors.at(color));
		_quad.setPosition(_board.getTilePosition(coord));
	}

	void RenderedFortress::setCoord(Coordinate coord)
	{
		Fortress::setCoord(coord);
		_quad.setPosition(_board.getTilePosition(coord));
	}
}
