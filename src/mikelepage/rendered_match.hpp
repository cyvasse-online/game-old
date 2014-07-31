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

#include <array>
#include <set>
#include <fea/rendering/quad.hpp>
#include <fea/rendering/renderer2d.hpp>
#include "hexagon_board.hpp"

class IngameState;

namespace mikelepage
{
	using cyvmath::PieceType;
	using cyvmath::PlayersColor;
	using cyvmath::mikelepage::Coordinate;
	using cyvmath::mikelepage::Match;
	using cyvmath::mikelepage::Player;
	using cyvmath::mikelepage::Piece;

	class LocalPlayer;
	class RenderedPiece;

	class RenderedMatch : public Match
	{
		public:
			typedef HexagonBoard<6> Board;

		private:
			fea::Renderer2D& _renderer;
			IngameState& _ingameState;

			Board _board;

			bool _gameEnded;
			std::string _status;

			std::shared_ptr<LocalPlayer> _self;
			std::shared_ptr<Player> _op;

			const PlayersColor _ownColor, _opColor;

			std::vector<fea::Quad*> _piecesToRender;
			std::vector<fea::Quad*> _terrainToRender;

			bool _setupAccepted;

			fea::Texture _buttonSetupDoneTexture;
			fea::Quad _buttonSetupDone;

			std::array<fea::Quad, 2> _dragonTiles;
			std::array<bool, 2> _hoveringDragonTile;

			std::array<fea::Quad, 3> _piecePromotionBackground;
			std::array<fea::Quad*, 3> _piecePromotionPieces;
			std::array<PieceType, 3> _piecePromotionTypes;
			uint_least8_t _renderPiecePromotionBgs;
			uint_least8_t _piecePromotionHover, _piecePromotionMousePress;

			std::vector<std::shared_ptr<fea::Quad>> _fortressReplacementTileHighlightings;

			std::shared_ptr<Piece> _selectedPiece;
			std::set<Coordinate> _possibleTargetTiles;

		public:
			RenderedMatch(IngameState&, fea::Renderer2D&, PlayersColor);

			// non-copyable
			RenderedMatch(const RenderedMatch&) = delete;
			const RenderedMatch& operator= (const RenderedMatch&) = delete;

			Board& getBoard()
			{ return _board; }

			const std::string& getStatus();
			void setStatus(const std::string&);

			void tick();

			void onTileClicked(Coordinate);
			void onMouseMoveOutsideBoard(const fea::Event::MouseMoveEvent&);
			void onClickedOutsideBoard(const fea::Event::MouseButtonEvent&);

			void tickPromotionPieceSelect();

			void onMouseMovedPromotionPieceSelect(const fea::Event::MouseMoveEvent&);
			void onMouseButtonPressedPromotionPieceSelect(const fea::Event::MouseButtonEvent&);
			void onMouseButtonReleasedPromotionPieceSelect(const fea::Event::MouseButtonEvent&);
			void onTileClickedFortressReplaceSelect(Coordinate);

			void placePiece(std::shared_ptr<RenderedPiece>, std::shared_ptr<Player>);
			void placePiecesSetup();

			void tryLeaveSetup();
			bool tryMovePiece(std::shared_ptr<Piece>, Coordinate);
			void addToBoard(PieceType, PlayersColor, Coordinate) final override;
			void removeFromBoard(std::shared_ptr<Piece>) final override;

			bool addFortressReplacementTile(Coordinate);
			void removeTerrain(fea::Quad*);

			void updateTurnStatus();
			void resetSelectedTile();
			void showPossibleTargetTiles();
			void clearPossibleTargetTiles();

			void showPromotionPieces(std::set<PieceType>);
			void chooseFortressReplacementTile();
			void endGame(PlayersColor winner) final override;
	};
}

#endif // _RENDERED_MATCH_HPP_
