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

#ifndef _MIKELEPAGE_RENDERED_PIECE_HPP_
#define _MIKELEPAGE_RENDERED_PIECE_HPP_

#include <cyvmath/mikelepage/piece.hpp>
#include <fea/rendering/quad.hpp>

#include "hexagon_board.hpp"

namespace mikelepage
{
	using cyvmath::PieceType;
	using cyvmath::PlayersColor;
	using cyvmath::mikelepage::PieceMap;

	class RenderedPiece : public cyvmath::mikelepage::Piece, public fea::Quad
	{
		public:
			typedef HexagonBoard<6> Board;

		private:
			Board& _board;

		public:
			RenderedPiece(PieceType, std::unique_ptr<Board::Coordinate>, PlayersColor, PieceMap&, Board&);

			bool moveTo(Board::Coordinate, bool checkMoveValidity) final override;
	};

	typedef std::vector<std::shared_ptr<RenderedPiece>> RenderedPieceVec;
}

#endif // _MIKELEPAGE_RENDERED_PIECE_HPP_
