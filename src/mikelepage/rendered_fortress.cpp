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

#include <array>
#include "hexagon_board.hpp"
#include "texturemaker.hpp" // lodepng helper function

using namespace std;
using namespace cyvmath;

namespace mikelepage
{
	RenderedFortress::RenderedFortress(PlayersColor color, Coordinate coord, HexagonBoard<6>& board)
		: Fortress(color, coord)
		, m_board(board)
	{
		assert(color != PlayersColor::UNDEFINED);

		string texturePath = "res/icons/" + string(PlayersColorToStr(color)) + "/fortress.png";
		m_texture = makeTexture(texturePath).first;
		m_quad.setTexture(m_texture);

		glm::vec2 tileSize = board.getTileSize();
		glm::vec2 tilePos = board.getTilePosition(coord);

		m_quad.setSize({tileSize.x, tileSize.y});
		m_quad.setPosition({tilePos.x, tilePos.y});
	}

	void RenderedFortress::setCoord(Coordinate coord)
	{
		m_coord = coord;

		glm::vec2 tilePos = m_board.getTilePosition(coord);
		m_quad.setPosition({tilePos.x, tilePos.y});
	}

	void RenderedFortress::ruined()
	{
		Fortress::ruined();

		string texturePath = "res/icons/" + string(PlayersColorToStr(m_color)) + "/fortress_ruined.png";
		m_texture = makeTexture(texturePath).first;
	}
}
