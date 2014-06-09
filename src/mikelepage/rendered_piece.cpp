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

using namespace cyvmath;
using namespace cyvmath::mikelepage;

namespace mikelepage
{
	RenderedPiece::RenderedPiece(PieceType type, Coordinate* coord,
	                             PlayersColor color, PieceMap& map, Board& board)
		: Piece(color, type, coord, map)
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

		glm::vec2 position = _board.getTilePosition(*_coord);
		// TODO: piece graphics should be scaled, after
		// that this constant should also be changed
		position += glm::vec2(8, 4);

		setPosition(position);
	}

	void RenderedPiece::moveTo(Coordinate coord, bool setup)
	{
		if(!setup)
		{
			// Check if the movement is legal
			// (Use assert for the check or return if the check fails)
			// TODO
		}

		PieceMap::iterator it = _map.find(*_coord);
		assert(it != _map.end());

		_coord = std::unique_ptr<Coordinate>(new Coordinate(coord));

		assert(&*it->second == this);
		std::pair<PieceMap::iterator, bool> res = _map.emplace(*_coord, it->second);
		_map.erase(it);
		// assert there is no other piece already on coord.
		// the check for that is probably better to do outside this
		// functions, but this check may be removed in the future.
		assert(res.second);

		glm::vec2 position = _board.getTilePosition(*_coord);
		position += glm::vec2(8, 4); // TODO

		setPosition(position);
	}
}
