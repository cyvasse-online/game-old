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
#include <fea/ui/event.hpp>

// higher priority (bigger enum value) means rendered later -> on top
enum class RenderPriority
{
	TERRAIN,
	PIECE,
	FORTRESS
};

template <int l> class HexagonBoard;
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
			fea::Renderer2D& m_renderer;
			IngameState& m_ingameState;

			std::unique_ptr<Board> m_board;

			bool m_gameEnded;
			std::string m_status;

			const PlayersColor m_ownColor, m_opColor;

			LocalPlayer& m_self;
			Player& m_op;

			std::map<RenderPriority, std::vector<fea::Drawable2D*>> m_renderedEntities;

			bool m_setupAccepted;

			fea::Texture m_buttonSetupDoneTexture;
			fea::Quad m_buttonSetupDone;

			std::array<fea::Quad, 3> m_piecePromotionBackground;
			std::array<fea::Quad*, 3> m_piecePromotionPieces;
			std::array<PieceType, 3> m_piecePromotionTypes;
			uint_least8_t m_renderPiecePromotionBgs;
			uint_least8_t m_piecePromotionHover, m_piecePromotionMousePress;

			std::shared_ptr<Piece> m_hoveredPiece, m_selectedPiece;

		public:
			RenderedMatch(IngameState&, fea::Renderer2D&, PlayersColor);

			// non-copyable
			RenderedMatch(const RenderedMatch&) = delete;
			RenderedMatch& operator=(const RenderedMatch&) = delete;

			Board& getBoard()
			{ return *m_board; }

			const std::string& getStatus()
			{ return m_status; }

			void setStatus(const std::string&);

			void tick();

			void onTileMouseOver(Coordinate);
			void onTileClicked(Coordinate);
			void onMouseMoveOutside(const fea::Event::MouseMoveEvent&);
			void onClickedOutsideBoard(const fea::Event::MouseButtonEvent&);

			void tickPromotionPieceSelect();

			void onMouseMovedPromotionPieceSelect(const fea::Event::MouseMoveEvent&);
			void onMouseButtonPressedPromotionPieceSelect(const fea::Event::MouseButtonEvent&);
			void onMouseButtonReleasedPromotionPieceSelect(const fea::Event::MouseButtonEvent&);

			void placePiece(std::shared_ptr<RenderedPiece>);
			void placePiecesSetup();

			void tryLeaveSetup();
			bool tryMovePiece(std::shared_ptr<Piece>, Coordinate);
			void addToBoard(PieceType, PlayersColor, Coordinate) final override;
			void removeFromBoard(std::shared_ptr<Piece>) final override;

			void updateTurnStatus();
			void showPossibleTargetTiles();

			void showPromotionPieces(std::set<PieceType>);
			void endGame(PlayersColor winner) final override;
	};
}

#endif // _RENDERED_MATCH_HPP_
