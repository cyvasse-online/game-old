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

// lodepng helper function
#include "texturemaker.hpp"

using namespace cyvmath::mikelepage;

namespace mikelepage
{
	RenderedPiece::RenderedPiece(PieceType type, dc::unique_ptr<Coordinate>&& coord,
	                             PlayersColor color, PieceMap& map, Board& board)
		: Piece(color, type, std::move(coord), map)
		, fea::Quad({48.0f, 40.0f})
		, _board(board)
	{
		static std::map<PieceType, std::string> fileNames = {
				{PIECE_MOUNTAIN,    "mountain.png"},
				{PIECE_RABBLE,      "rabble.png"},
				{PIECE_CROSSBOWS,   "crossbows.png"},
				{PIECE_SPEARS,      "spears.png"},
				{PIECE_LIGHT_HORSE, "light_horse.png"},
				{PIECE_TREBUCHET,   "trebuchet.png"},
				{PIECE_ELEPHANT,    "elephant.png"},
				{PIECE_HEAVY_HORSE, "heavy_horse.png"},
				{PIECE_DRAGON,      "dragon.png"},
				{PIECE_KING,        "king.png"}
			};

		std::string texturePath = "icons/" + (std::string(PlayersColorToStr(color))) + "/" + fileNames.at(type);
		mTexture = new fea::Texture(makeTexture(texturePath, 48, 40));

		glm::vec2 position = _board.getTilePosition(*dynamic_cast<Coordinate*>(_coord.get()));
		// TODO: piece graphics should be scaled, after
		// that this constant should also be changed
		position += glm::vec2(8, 4);

		setPosition(position);
	}

	bool RenderedPiece::moveTo(const cyvmath::CoordinateDcUqP& coord, bool checkMoveValidity)
	{
		if(!Piece::moveTo(std::move(coord), checkMoveValidity))
			return false;

		glm::vec2 position = _board.getTilePosition(*dynamic_cast<Coordinate*>(_coord.get()));
		position += glm::vec2(8, 4); // TODO

		setPosition(position);

		return true;
	}
}
