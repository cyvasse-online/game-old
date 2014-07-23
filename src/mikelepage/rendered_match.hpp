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

#ifndef _RENDERED_MATCH_HPP_
#define _RENDERED_MATCH_HPP_

#include <cyvmath/mikelepage/match.hpp>

#include <set>
#include <fea/rendering/quad.hpp>
#include <fea/rendering/renderer2d.hpp>
#include <cyvmath/mikelepage/piece.hpp>
#include "hexagon_board.hpp"

class IngameState;

namespace mikelepage
{
	using cyvmath::PlayersColor;
	using cyvmath::mikelepage::Coordinate;
	using cyvmath::mikelepage::Match;
	using cyvmath::mikelepage::Piece;
	using cyvmath::mikelepage::PieceMap;

	class LocalPlayer;
	class RenderedPiece;

	class RenderedMatch : public Match
	{
		public:
			typedef HexagonBoard<6> Board;
			typedef std::vector<std::shared_ptr<RenderedPiece>> RenderedPieceVec;

		private:
			fea::Renderer2D& _renderer;

			Board _board;

			std::shared_ptr<LocalPlayer> _self;

			RenderedPieceVec _piecesToRender;

			bool _setupAccepted;

			fea::Texture _buttonSetupDoneTexture;
			fea::Quad _buttonSetupDone;

			std::shared_ptr<Piece> _selectedPiece;
			std::set<Coordinate> _possibleTargetTiles;
		public:
			RenderedMatch(IngameState&, fea::Renderer2D&, PlayersColor firstPlayer);
			~RenderedMatch() = default;

			// non-copyable
			RenderedMatch(const RenderedMatch&) = delete;
			const RenderedMatch& operator= (const RenderedMatch&) = delete;

			Board& getBoard()
			{ return _board; }

			void tick();

			void onTileClicked(Coordinate);
			void onClickedOutsideBoard(const fea::Event::MouseButtonEvent&);

			void placePiecesSetup();
			void tryLeaveSetup();
			void tryMovePiece(std::shared_ptr<Piece>, Coordinate);
			void showPossibleTargetTiles(Coordinate);
			void clearPossibleTargetTiles();
	};
}

#endif // _RENDERED_MATCH_HPP_
