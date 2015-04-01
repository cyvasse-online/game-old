/* Copyright 2014 - 2015 Jonas Platte
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

#include <cyvasse/match.hpp>

#include <array>
#include <map>
#include <set>

#include <fea/rendering/quad.hpp>
#include <fea/rendering/renderer2d.hpp>
#include <fea/rendering/texture.hpp>
#include <fea/ui/event.hpp>

#include <optional.hpp>
#include "hexagon_board.hpp"

// higher priority (bigger enum value) means rendered later -> on top
enum class RenderPriority
{
	TERRAIN,
	PIECE,
	FORTRESS
};

class IngameState;

class LocalPlayer;
class RenderedPiece;

class RenderedMatch : public cyvasse::Match
{
	public:
		typedef HexagonBoard<6> Board;

	private:
		fea::Renderer2D& m_renderer;
		IngameState& m_ingameState;

		Board m_board;

		bool m_gameEnded;
		std::string m_status;

		const cyvasse::PlayersColor m_ownColor, m_opColor;

		LocalPlayer& m_self;
		cyvasse::Player& m_op;

		std::map<RenderPriority, std::vector<fea::Drawable2D*>> m_renderedEntities;

		bool m_setupAccepted;

		fea::Texture m_buttonSetupDoneTexture;
		fea::Quad m_buttonSetupDone;

		std::shared_ptr<cyvasse::Piece> m_hoveredPiece, m_selectedPiece;

		struct PiecePromotionBox
		{
			fea::Quad bgQuad;
			fea::Quad pieceQuad;
			fea::Texture pieceTexture;

			PiecePromotionBox(const glm::vec2& pos, const glm::vec2& pieceSize, fea::Texture&&);
		};

		std::map<cyvasse::PieceType, PiecePromotionBox> m_piecePromotionBoxes;
		optional<cyvasse::PieceType> m_piecePromotionHover;

	public:
		RenderedMatch(IngameState&, fea::Renderer2D&, cyvasse::PlayersColor);

		// non-copyable
		RenderedMatch(const RenderedMatch&) = delete;
		RenderedMatch& operator=(const RenderedMatch&) = delete;

		Board& getBoard()
		{ return m_board; }

		const std::string& getStatus()
		{ return m_status; }

		void setStatus(const std::string&);

		void tick();

		void onTileMouseOver(cyvasse::HexCoordinate<6>);
		void onTileClicked(cyvasse::HexCoordinate<6>);
		void onMouseMoveOutside(const fea::Event::MouseMoveEvent&);
		void onClickedOutsideBoard(const fea::Event::MouseButtonEvent&);

		void tickPromotionPieceSelect();

		void onPromotionPieceMouseMotion(const fea::Event::MouseMoveEvent&);
		void onPromotionPieceClick(const fea::Event::MouseButtonEvent&);

		void placePiece(std::shared_ptr<RenderedPiece>);
		void placePiecesSetup();

		void tryLeaveSetup();
		bool tryMovePiece(std::shared_ptr<cyvasse::Piece>, cyvasse::HexCoordinate<6>);
		virtual void addToBoard(cyvasse::PieceType, cyvasse::PlayersColor, cyvasse::HexCoordinate<6>) final override;
		virtual void removeFromBoard(std::shared_ptr<cyvasse::Piece>) final override;
		virtual void endGame(cyvasse::PlayersColor winner) final override;

		void updateTurnStatus();
		void showPossibleTargetTiles();

		void showPromotionPieces(std::set<cyvasse::PieceType>);
};

#endif // _RENDERED_MATCH_HPP_
