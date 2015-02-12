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

#include "rendered_piece.hpp"

#include <cyvmath/mikelepage/match.hpp>
#include "hexagon_board.hpp"
#include "texturemaker.hpp" // lodepng helper function
#include "rendered_match.hpp"

using namespace std;
using namespace cyvmath;

namespace mikelepage
{
	RenderedPiece::RenderedPiece(PieceType type, Coordinate coord, PlayersColor color, RenderedMatch& match)
		: Piece(color, type, coord, match)
		, m_board(match.getBoard())
		, m_quad(m_board.getTileSize())
	{
		static const map<PieceType, string> fileNames {
				{PieceType::MOUNTAINS,   "mountains.png"},
				{PieceType::RABBLE,      "rabble.png"},
				{PieceType::CROSSBOWS,   "crossbows.png"},
				{PieceType::SPEARS,      "spears.png"},
				{PieceType::LIGHT_HORSE, "light_horse.png"},
				{PieceType::TREBUCHET,   "trebuchet.png"},
				{PieceType::ELEPHANT,    "elephant.png"},
				{PieceType::HEAVY_HORSE, "heavy_horse.png"},
				{PieceType::DRAGON,      "dragon.png"},
				{PieceType::KING,        "king.png"}
			};

		string texturePath = "res/icons/" + string(PlayersColorToStr(color)) + "/" + fileNames.at(type);
		m_texture = makeTexture(texturePath).first;

		m_quad.setTexture(m_texture);
		m_quad.setPosition(m_board.getTilePosition(*m_coord));
	}

	bool RenderedPiece::moveTo(Coordinate coord, bool setup)
	{
		if(!Piece::moveTo(coord, setup))
			return false;

		m_quad.setPosition(m_board.getTilePosition(coord));

		return true;
	}
}
