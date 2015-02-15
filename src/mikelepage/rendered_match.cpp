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

#include "rendered_match.hpp"

#include <algorithm>
#include <fstream>
#include <stdexcept>
#ifdef EMSCRIPTEN
	#include <emscripten.h>
#endif
#include <json/reader.h>
#include <cyvws/json_game_msg.hpp>
#include <make_unique.hpp>
#include <texturemaker.hpp> // lodepng helper function
#include "common.hpp"
#include "cyvasse_ws_client.hpp"
#include "hexagon_board.hpp"
#include "ingame_state.hpp"
#include "local_player.hpp"
#include "rendered_fortress.hpp"
#include "rendered_piece.hpp"
#include "rendered_terrain.hpp"
#include "remote_player.hpp"

using namespace std;
using namespace std::placeholders;
using namespace cyvmath;
using namespace cyvws;

namespace mikelepage
{
	using Hexagon = Hexagon<6>;
	using HexCoordinate = Hexagon::Coordinate;

	static Match::playerArray createPlayerArray(PlayersColor localPlayersColor, RenderedMatch& match)
	{
		auto remotePlayersColor = !localPlayersColor;

		auto localPlayer  = make_unique<LocalPlayer>(localPlayersColor, match);
		auto remotePlayer = make_unique<RemotePlayer>(remotePlayersColor, match);

		if (localPlayersColor < remotePlayersColor)
			return {{move(localPlayer), move(remotePlayer)}};
		else
			return {{move(remotePlayer), move(localPlayer)}};
	}

	RenderedMatch::RenderedMatch(IngameState& ingameState, fea::Renderer2D& renderer, PlayersColor color)
		: cyvmath::mikelepage::Match({}, false, false, createPlayerArray(color, *this)) // TODO
		, m_renderer{renderer}
		, m_ingameState{ingameState}
		, m_board{make_unique<Board>(renderer, color)}
		, m_gameEnded{false}
		, m_ownColor{color}
		, m_opColor{!color}
		, m_self{dynamic_cast<LocalPlayer&>(*m_players[m_ownColor])}
		, m_op{dynamic_cast<RemotePlayer&>(*m_players[m_opColor])}
		, m_setupAccepted{false}
		, m_piecePromotionBackground{{glm::vec2{100, 100}, glm::vec2{100, 100}, glm::vec2{100, 100}}}
		, m_piecePromotionTypes{{PieceType::UNDEFINED, PieceType::UNDEFINED, PieceType::UNDEFINED}}
		, m_renderPiecePromotionBgs{0}
		, m_piecePromotionHover{0}
		, m_piecePromotionMousePress{0}
	{
		// hardcoded temporarily [TODO]
		static const map<PlayersColor, Coordinate> fortressStartCoords {
			{PlayersColor::WHITE, HexCoordinate(4, 7)},
			{PlayersColor::BLACK, HexCoordinate(6, 3)}
		};

		auto ownFortress = make_unique<RenderedFortress>(m_ownColor, fortressStartCoords.at(m_ownColor), *m_board);
		m_renderedEntities[RenderPriority::FORTRESS].push_back(ownFortress->getQuad());

		m_self.setFortress(move(ownFortress));
		m_op.setFortress(make_unique<RenderedFortress>(m_opColor, fortressStartCoords.at(m_opColor), *m_board));

		glm::uvec2 boardSize = m_board->getSize();
		glm::uvec2 boardPos = m_board->getPosition();

		auto tmpTexture = makeTexture("res/setup-done.png");

		m_buttonSetupDoneTexture = move(tmpTexture.first);
		m_buttonSetupDone.setPosition(boardPos + boardSize - tmpTexture.second);
		m_buttonSetupDone.setSize(tmpTexture.second); // hardcoded for now, can be done properly somewhen else
		m_buttonSetupDone.setTexture(m_buttonSetupDoneTexture);

		for (auto& quad : m_piecePromotionBackground)
			quad.setColor({95, 95, 95});

		placePiecesSetup();

		ingameState.tick                  = bind(&RenderedMatch::tick, this);
		ingameState.onMouseMoved          = bind(&Board::onMouseMoved, m_board.get(), _1);
		ingameState.onMouseButtonPressed  = bind(&Board::onMouseButtonPressed, m_board.get(), _1);
		ingameState.onMouseButtonReleased = bind(&Board::onMouseButtonReleased, m_board.get(), _1);
		ingameState.onKeyPressed          = [](const fea::Event::KeyEvent&) { };
		ingameState.onKeyReleased         = [](const fea::Event::KeyEvent&) { };

		m_board->onTileMouseOver    = bind(&RenderedMatch::onTileMouseOver, this, _1);
		m_board->onTileClicked      = bind(&RenderedMatch::onTileClicked, this, _1);
		m_board->onMouseMoveOutside = bind(&RenderedMatch::onMouseMoveOutside, this, _1);
		m_board->onClickedOutside   = bind(&RenderedMatch::onClickedOutsideBoard, this, _1);

		setStatus("Setup");
	}

	void RenderedMatch::setStatus(const string& text)
	{
		if (!m_gameEnded)
		{
			m_status = text;

			#ifdef EMSCRIPTEN
			EM_ASM_({
				Module.setStatus(Module.Pointer_stringify($0));
			}, text.c_str());
			#endif
		}
	}

	void RenderedMatch::tick()
	{
		m_board->tick();

		for (auto&& entityVecIt : m_renderedEntities)
			for (auto&& it : entityVecIt.second)
				m_renderer.queue(*it);

		// Move the logic bit to the backend
		if (m_setup && m_self.setupComplete() && !m_setupAccepted)
			m_renderer.queue(m_buttonSetupDone);
		else if (m_renderPiecePromotionBgs > 0)
		{
			for (int i = 0; i < m_renderPiecePromotionBgs; ++i)
			{
				m_renderer.queue(m_piecePromotionBackground[i]);
				m_renderer.queue(*m_piecePromotionPieces[i]);
			}
		}
	}

	void RenderedMatch::onTileMouseOver(Coordinate coord)
	{
		// if one of the players is still in setup,
		// or a piece is selected, do nothing
		if (m_setup || m_selectedPiece)
			return;

		// search for a piece on the clicked tile
		auto it = m_activePieces.find(coord);

		if (it != m_activePieces.end() &&
		   it->second->getType() != PieceType::MOUNTAINS)
		{
			// a piece of the player was hovered

			if (it->second != m_hoveredPiece)
			{
				m_hoveredPiece = it->second;

				showPossibleTargetTiles();
			}
		}
		else if (m_hoveredPiece)
		{
			m_hoveredPiece.reset();
			m_board->clearHighlighting(HighlightingId::PTT);
		}
	}

	void RenderedMatch::onTileClicked(Coordinate coord)
	{
		// if the local player finished setting up,
		// but the remote player didn't yet
		if (m_setupAccepted && m_setup)
			return;

		if (!m_setup && m_activePlayer != m_ownColor)
			return;

		if (!m_selectedPiece)
		{
			// search for a piece on the clicked tile
			auto it = m_activePieces.find(coord);

			if (it != m_activePieces.end() &&
			   it->second->getColor() == m_ownColor &&
			   (m_setup || it->second->getType() != PieceType::MOUNTAINS))
			{
				// a piece of the player was clicked
				m_selectedPiece = it->second;

				m_board->highlightTile(coord, HighlightingId::SEL);

				// assert target tiles are already shown through the hover stuff
			}
		}
		else // a piece is selected
		{
			// determine which piece is on the clicked tile
			shared_ptr<cyvmath::mikelepage::Piece> piece;

			auto pieceIt = m_activePieces.find(coord);
			if (pieceIt != m_activePieces.end())
				piece = pieceIt->second;

			if (!piece || piece->getColor() == m_opColor)
			{
				auto oldCoord = *m_selectedPiece->getCoord();

				if (tryMovePiece(m_selectedPiece, coord))
				{
					// move is valid and was done

					if (!m_setup)
					{
						if (piece)
							CyvasseWSClient::instance().send(json::gameMsgMoveCapture(
								m_selectedPiece->getType(), oldCoord, coord, piece->getType(), coord
							));
						else
							CyvasseWSClient::instance().send(json::gameMsgMove(
								m_selectedPiece->getType(), oldCoord, coord
							));
					}

					if (!m_setupAccepted)
						m_self.checkSetupComplete();

					m_selectedPiece.reset();
					m_board->clearHighlighting(HighlightingId::SEL);

					if (!m_setup)
						showPossibleTargetTiles();
				}
			}
			else if (piece->getType() != PieceType::MOUNTAINS || m_setup)
			{
				// there is a piece of the player on the clicked tile
				auto selectedCoord = m_selectedPiece->getCoord();
				assert(selectedCoord);

				m_selectedPiece.reset();
				m_board->clearHighlighting(HighlightingId::SEL);

				if (coord != *selectedCoord)
				{
					// another piece clicked, create a new highlightning
					m_selectedPiece = piece;
					m_hoveredPiece = piece;

					m_board->highlightTile(coord, HighlightingId::SEL);
					if (!m_setup)
						showPossibleTargetTiles();
				}
			}
		}
	}

	void RenderedMatch::onMouseMoveOutside(const fea::Event::MouseMoveEvent&)
	{
		if (m_hoveredPiece)
		{
			m_hoveredPiece.reset();

			if (!m_selectedPiece)
				m_board->clearHighlighting(HighlightingId::PTT);
		}
	}

	void RenderedMatch::onClickedOutsideBoard(const fea::Event::MouseButtonEvent& event)
	{
		// 'Setup done' button clicked (while being visible)
		if (m_self.setupComplete() && !m_setupAccepted && mouseOver(m_buttonSetupDone, {event.x, event.y}))
		{
			m_setupAccepted = true;

			// send before modifying m_activePieces, so the map doesn't
			// have to be filtered for only black / white pieces
			assert(m_activePieces.size() == 26);
			CyvasseWSClient::instance().send(json::gameMsgSetIsReady());
			CyvasseWSClient::instance().send(json::gameMsgSetOpeningArray(m_activePieces));

			tryLeaveSetup();
		}

		if (m_selectedPiece)
		{
			m_selectedPiece.reset();
			m_board->clearHighlighting(HighlightingId::SEL);
			m_board->clearHighlighting(HighlightingId::PTT);
		}
	}

	void RenderedMatch::onMouseMovedPromotionPieceSelect(const fea::Event::MouseMoveEvent& mouseMove)
	{
		bool hoverOne = false;

		for (int i = 0; i < m_renderPiecePromotionBgs; i++)
			if (mouseOver(m_piecePromotionBackground[i], {mouseMove.x, mouseMove.y}))
			{
				hoverOne = true;

				m_piecePromotionHover = i+1;
				m_piecePromotionBackground[i].setColor({127, 127, 127});
			}
			else
			{
				m_piecePromotionBackground[i].setColor({95, 95, 95});
			}

		if (!hoverOne)
			m_piecePromotionHover = 0;
	}

	void RenderedMatch::onMouseButtonPressedPromotionPieceSelect(const fea::Event::MouseButtonEvent& mouseButton)
	{
		if (mouseButton.button != fea::Mouse::Button::LEFT || m_piecePromotionMousePress)
			return;

		if (m_piecePromotionHover)
			m_piecePromotionMousePress = m_piecePromotionHover;
	}

	void RenderedMatch::onMouseButtonReleasedPromotionPieceSelect(const fea::Event::MouseButtonEvent& mouseButton)
	{
		if (mouseButton.button != fea::Mouse::Button::LEFT || !m_piecePromotionMousePress)
			return;

		if (m_piecePromotionMousePress == m_piecePromotionHover)
		{
			auto piece = getPieceAt(m_self.getFortress().getCoord());
			assert(piece);

			PieceType origType = piece->getType();
			PieceType newType = m_piecePromotionTypes[m_piecePromotionMousePress-1];

			piece->promoteTo(newType);
			CyvasseWSClient::instance().send(json::gameMsgPromote(origType, newType));

			m_renderPiecePromotionBgs = 0;
			m_piecePromotionPieces.fill(nullptr);

			m_ingameState.onMouseMoved          = bind(&Board::onMouseMoved, m_board.get(), _1);
			m_ingameState.onMouseButtonPressed  = bind(&Board::onMouseButtonPressed, m_board.get(), _1);
			m_ingameState.onMouseButtonReleased = bind(&Board::onMouseButtonReleased, m_board.get(), _1);
		}

		m_piecePromotionMousePress = 0;
	}

	void RenderedMatch::placePiece(shared_ptr<RenderedPiece> piece)
	{
		using cyvmath::mikelepage::TerrainType;

		auto coord = piece->getCoord();
		assert(coord);

		m_activePieces.emplace(*coord, piece);

		TerrainType tType = piece->getSetupTerrain();

		if (tType != TerrainType::UNDEFINED)
		{
			auto terrain = make_shared<RenderedTerrain>(tType, *coord, *m_board, m_terrain);

			m_terrain.emplace(*coord, terrain);
			m_renderedEntities[RenderPriority::TERRAIN].push_back(terrain->getQuad());
		}

		m_renderedEntities[RenderPriority::PIECE].push_back(piece->getQuad());
	}

	void RenderedMatch::placePiecesSetup()
	{
		string filePath = "res/start-positions/" + PlayersColorToStr(m_ownColor) + ".json";

		ifstream ifs(filePath);
		if (!ifs)
			throw runtime_error("Couldn't open \"" + filePath + "\"!");

		Json::Value val;
		auto success = Json::Reader().parse(ifs, val, false);
		assert(success);

		auto oArr = json::pieceMap(val);

		for (const auto& it : oArr)
		{
			for (const auto& coord : it.second)
				placePiece(make_shared<RenderedPiece>(
					it.first, coord, m_ownColor, *this
				));
		}
	}

	void RenderedMatch::tryLeaveSetup()
	{
		if (!m_setupAccepted) return;

		for (auto&& player : m_players)
			if (!player->setupComplete())
				return;

		m_setup = false;

		// TODO: rewrite the following stuff when adding a bot
		auto& op = dynamic_cast<RemotePlayer&>(m_op);

		auto& opFortress = dynamic_cast<RenderedFortress&>(op.getFortress());
		m_renderedEntities[RenderPriority::FORTRESS].push_back(opFortress.getQuad());

		for (auto&& piece : op.getPieceCache())
			placePiece(piece);

		op.clearPieceCache();

		m_bearingTable.init();

		m_board->clearHighlighting(HighlightingId::DIM);

		updateTurnStatus();
	}

	bool RenderedMatch::tryMovePiece(shared_ptr<cyvmath::mikelepage::Piece> piece, Coordinate coord)
	{
		assert(piece);

		auto oldCoord = piece->getCoord();

		if (piece->moveTo(coord, m_setup))
		{
			m_board->clearHighlighting(HighlightingId::PTT);

			if (!m_setup)
			{
				dynamic_cast<cyvmath::mikelepage::Player&>(*m_players[m_activePlayer]).onTurnEnd();

				m_activePlayer = !m_activePlayer;

				updateTurnStatus();

				if (m_activePlayer == m_ownColor)
					m_self.onTurnBegin();

				array<Coordinate, 2> coords = {{coord, *oldCoord}};
				m_board->highlightTiles(coords.begin(), coords.end(), HighlightingId::LAST_MOVE);
			}

			return true;
		}

		return false;
	}

	void RenderedMatch::addToBoard(PieceType type, PlayersColor color, const HexCoordinate& coord)
	{
		Match::addToBoard(type, color, coord);

		auto rPiece = dynamic_pointer_cast<RenderedPiece>(getPieceAt(coord));
		assert(rPiece);

		rPiece->setPosition(m_board->getTileAt(coord)->getPosition());

		m_renderedEntities[RenderPriority::PIECE].push_back(rPiece->getQuad());
	}

	void RenderedMatch::removeFromBoard(shared_ptr<cyvmath::mikelepage::Piece> piece)
	{
		Match::removeFromBoard(piece);

		// TODO: place the piece somewhere outside
		// the board instead of not rendering it

		auto rPiece = dynamic_pointer_cast<RenderedPiece>(piece);
		assert(rPiece);

		auto& piecesToRender = m_renderedEntities[RenderPriority::PIECE];

		auto it = find(piecesToRender.begin(), piecesToRender.end(), rPiece->getQuad());
		assert(it != piecesToRender.end());
		piecesToRender.erase(it);
	}

	void RenderedMatch::updateTurnStatus()
	{
		setStatus(PlayersColorToPrettyStr(m_activePlayer) + "'s turn");
	}

	void RenderedMatch::showPossibleTargetTiles()
	{
		assert(!m_setup);

		if (!m_hoveredPiece)
			return;

		// should this be removed?
		// if not, merge with the above
		if (m_gameEnded)
			return;

		// the tile clicked on holds a piece of the player
		auto pTT = m_hoveredPiece->getPossibleTargetTiles();
		m_board->highlightTiles(pTT.begin(), pTT.end(), HighlightingId::PTT);
	}

	void RenderedMatch::showPromotionPieces(set<PieceType> pieceTypes)
	{
		assert(pieceTypes.size() > 1 && pieceTypes.size() <= 3);

		m_renderPiecePromotionBgs = pieceTypes.size();
		glm::uvec2 scrMid = m_renderer.getViewport().getSize() / glm::uvec2{2, 2};

		if (pieceTypes.size() == 2)
		{
			m_piecePromotionBackground[0].setPosition(glm::ivec2(scrMid) + glm::ivec2{-100, -50});
			m_piecePromotionBackground[1].setPosition(glm::ivec2(scrMid) + glm::ivec2{0, -50});
		}
		else
		{
			m_piecePromotionBackground[0].setPosition(glm::ivec2(scrMid) + glm::ivec2{-150, -50});
			m_piecePromotionBackground[1].setPosition(glm::ivec2(scrMid) + glm::ivec2{-50, -50});
			m_piecePromotionBackground[2].setPosition(glm::ivec2(scrMid) + glm::ivec2{50, -50});
		}

		auto& inactivePieces = m_self.getInactivePieces();

		uint_least8_t i = 0;
		for (PieceType type : pieceTypes)
		{
			m_piecePromotionTypes[i] = type;

			auto it = inactivePieces.find(type);
			assert(it != inactivePieces.end());

			auto rPiece = dynamic_pointer_cast<RenderedPiece>(it->second);
			assert(rPiece);

			rPiece->setPosition(m_piecePromotionBackground[i].getPosition() + glm::vec2{17, 25}); // TODO
			m_piecePromotionPieces[i] = rPiece->getQuad();
			i++;
		}

		m_ingameState.onMouseMoved          = bind(&RenderedMatch::onMouseMovedPromotionPieceSelect, this, _1);
		m_ingameState.onMouseButtonPressed  = bind(&RenderedMatch::onMouseButtonPressedPromotionPieceSelect, this, _1);
		m_ingameState.onMouseButtonReleased = bind(&RenderedMatch::onMouseButtonReleasedPromotionPieceSelect, this, _1);
	}

	void RenderedMatch::endGame(PlayersColor winner)
	{
		setStatus(PlayersColorToPrettyStr(winner) + " won!");

		m_board->clearHighlighting(HighlightingId::PTT);
		m_board->clearHighlighting(HighlightingId::HOVER);

		m_ingameState.onMouseMoved          = [](const fea::Event::MouseMoveEvent&) { };
		m_ingameState.onMouseButtonPressed  = [](const fea::Event::MouseButtonEvent&) { };
		m_ingameState.onMouseButtonReleased = [](const fea::Event::MouseButtonEvent&) { };
		m_ingameState.onKeyPressed          = [](const fea::Event::KeyEvent&) { };
		m_ingameState.onKeyReleased         = [](const fea::Event::KeyEvent&) { };

		m_gameEnded = true;
	}
}
