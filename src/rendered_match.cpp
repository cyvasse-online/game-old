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
#ifdef __EMSCRIPTEN__
	#include <emscripten.h>
#endif
#include <json/reader.h>
#include <cyvws/json_game_msg.hpp>
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
using namespace cyvasse;
using namespace cyvws;

using HexCoordinate = Hexagon<6>::Coordinate;

RenderedMatch::PiecePromotionBox::PiecePromotionBox(const glm::vec2& size, const glm::vec2& pos, fea::Texture&& texture)
	: bgQuad(glm::vec2(100, 100))
	, pieceQuad(size)
	, pieceTexture(std::move(texture))
{
	bgQuad.setPosition(pos);

	pieceQuad.setTexture(pieceTexture);
	pieceQuad.setPosition(glm::vec2(pos) + glm::vec2(17, 25)); // TODO
}

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
	: Match({}, false, false, createPlayerArray(color, *this)) // TODO
	, m_renderer{renderer}
	, m_ingameState{ingameState}
	, m_board(renderer, color)
	, m_gameEnded{false}
	, m_ownColor{color}
	, m_opColor{!color}
	, m_self{dynamic_cast<LocalPlayer&>(*m_players[m_ownColor])}
	, m_op{dynamic_cast<RemotePlayer&>(*m_players[m_opColor])}
	, m_setupAccepted{false}
{
	// hardcoded temporarily [TODO]
	static const map<PlayersColor, Coordinate> fortressStartCoords {
		{PlayersColor::WHITE, HexCoordinate(4, 7)},
		{PlayersColor::BLACK, HexCoordinate(6, 3)}
	};

	auto ownFortress = make_unique<RenderedFortress>(m_ownColor, fortressStartCoords.at(m_ownColor), m_board);
	m_renderedEntities[RenderPriority::FORTRESS].push_back(ownFortress->getQuad());

	m_self.setFortress(move(ownFortress));
	m_op.setFortress(make_unique<RenderedFortress>(m_opColor, fortressStartCoords.at(m_opColor), m_board));

	glm::uvec2 boardSize = m_board.getSize();
	glm::uvec2 boardPos = m_board.getPosition();

	auto tmpTexture = makeTexture("res/setup-done.png");

	m_buttonSetupDoneTexture = move(tmpTexture.first);
	m_buttonSetupDone.setPosition(boardPos + boardSize - tmpTexture.second);
	m_buttonSetupDone.setSize(tmpTexture.second); // hardcoded for now, can be done properly somewhen else
	m_buttonSetupDone.setTexture(m_buttonSetupDoneTexture);

	placePiecesSetup();

	ingameState.tick                  = bind(&RenderedMatch::tick, this);
	ingameState.onMouseMoved          = bind(&Board::onMouseMoved, &m_board, _1);
	ingameState.onMouseButtonReleased = bind(&Board::onMouseButtonReleased, &m_board, _1);

	m_board.onTileMouseOver    = bind(&RenderedMatch::onTileMouseOver, this, _1);
	m_board.onTileClicked      = bind(&RenderedMatch::onTileClicked, this, _1);
	m_board.onMouseMoveOutside = bind(&RenderedMatch::onMouseMoveOutside, this, _1);
	m_board.onClickedOutside   = bind(&RenderedMatch::onClickedOutsideBoard, this, _1);

	setStatus("Setup");
}

void RenderedMatch::setStatus(const string& text)
{
	if (!m_gameEnded)
	{
		m_status = text;

		#ifdef __EMSCRIPTEN__
		EM_ASM_({
			Module.setStatus(Module.Pointer_stringify($0));
		}, text.c_str());
		#endif
	}
}

void RenderedMatch::tick()
{
	m_board.tick();

	for (auto&& entityVecIt : m_renderedEntities)
		for (auto&& it : entityVecIt.second)
			m_renderer.queue(*it);

	// Move the logic bit to the backend
	if (m_setup && m_self.setupComplete() && !m_setupAccepted)
		m_renderer.queue(m_buttonSetupDone);
	else if (!m_piecePromotionBoxes.empty())
	{
		for (const auto& box : m_piecePromotionBoxes)
		{
			m_renderer.queue(box.second.bgQuad);
			m_renderer.queue(box.second.pieceQuad);
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
		m_board.clearHighlighting(HighlightingId::PTT);
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

			m_board.highlightTile(coord, HighlightingId::SEL);

			// assert target tiles are already shown through the hover stuff
		}
	}
	else // a piece is selected
	{
		// determine which piece is on the clicked tile
		shared_ptr<Piece> piece;

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
				m_board.clearHighlighting(HighlightingId::SEL);

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
			m_board.clearHighlighting(HighlightingId::SEL);

			if (coord != *selectedCoord)
			{
				// another piece clicked, create a new highlightning
				m_selectedPiece = piece;
				m_hoveredPiece = piece;

				m_board.highlightTile(coord, HighlightingId::SEL);
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
			m_board.clearHighlighting(HighlightingId::PTT);
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
		m_board.clearHighlighting(HighlightingId::SEL);
		m_board.clearHighlighting(HighlightingId::PTT);
	}
}

void RenderedMatch::onPromotionPieceMouseMotion(const fea::Event::MouseMoveEvent& mouseMove)
{
	bool hoverOne = false;

	for (auto&& box : m_piecePromotionBoxes)
		if (mouseOver(box.second.bgQuad, {mouseMove.x, mouseMove.y}))
		{
			hoverOne = true;

			m_piecePromotionHover = box.first;
			box.second.bgQuad.setColor({127, 127, 127});
		}
		else
		{
			box.second.bgQuad.setColor({95, 95, 95});
		}

	if (!hoverOne)
		m_piecePromotionHover = nullopt;
}

void RenderedMatch::onPromotionPieceClick(const fea::Event::MouseButtonEvent& mouseButton)
{
	if (mouseButton.button != fea::Mouse::Button::LEFT)
		return;

	auto piece = getPieceAt(m_self.getFortress().getCoord());
	assert(piece);

	PieceType origType = piece->getType();
	PieceType newType = *m_piecePromotionHover;

	piece->promoteTo(newType);
	CyvasseWSClient::instance().send(json::gameMsgPromote(origType, newType));

	m_piecePromotionBoxes.clear();

	m_ingameState.onMouseMoved          = bind(&Board::onMouseMoved, &m_board, _1);
	m_ingameState.onMouseButtonReleased = bind(&Board::onMouseButtonReleased, &m_board, _1);

	m_piecePromotionHover = nullopt;
}

void RenderedMatch::placePiece(shared_ptr<RenderedPiece> piece)
{
	auto coord = piece->getCoord();
	assert(coord);

	m_activePieces.emplace(*coord, piece);

	TerrainType tType = piece->getSetupTerrain();

	if (tType != TerrainType::UNDEFINED)
	{
		auto terrain = make_shared<RenderedTerrain>(tType, *coord, m_board, m_terrain);

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

	m_board.clearHighlighting(HighlightingId::DIM);

	updateTurnStatus();
}

bool RenderedMatch::tryMovePiece(shared_ptr<Piece> piece, Coordinate coord)
{
	assert(piece);

	auto oldCoord = piece->getCoord();

	if (piece->moveTo(coord, m_setup))
	{
		m_board.clearHighlighting(HighlightingId::PTT);

		if (!m_setup)
		{
			dynamic_cast<Player&>(*m_players[m_activePlayer]).onTurnEnd();

			m_activePlayer = !m_activePlayer;

			updateTurnStatus();

			if (m_activePlayer == m_ownColor)
				m_self.onTurnBegin();

			array<Coordinate, 2> coords = {{coord, *oldCoord}};
			m_board.highlightTiles(coords.begin(), coords.end(), HighlightingId::LAST_MOVE);
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

	rPiece->setPosition(m_board.getTileAt(coord)->getPosition());

	m_renderedEntities[RenderPriority::PIECE].push_back(rPiece->getQuad());
}

void RenderedMatch::removeFromBoard(shared_ptr<Piece> piece)
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

void RenderedMatch::endGame(PlayersColor winner)
{
	setStatus(PlayersColorToPrettyStr(winner) + " won!");

	m_board.clearHighlighting(HighlightingId::PTT);
	m_board.clearHighlighting(HighlightingId::HOVER);

	m_ingameState.onMouseMoved          = [](const fea::Event::MouseMoveEvent&) { };
	m_ingameState.onMouseButtonReleased = [](const fea::Event::MouseButtonEvent&) { };

	m_gameEnded = true;
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
	m_board.highlightTiles(pTT.begin(), pTT.end(), HighlightingId::PTT);
}

void RenderedMatch::showPromotionPieces(set<PieceType> pieceTypes)
{
	auto promotionOptionN = m_piecePromotionBoxes.size();
	int startX =
		(promotionOptionN == 2) ? -100 :
		(promotionOptionN == 3) ? -150 :
		(assert(0), 0);

	glm::ivec2 scrMid = m_renderer.getViewport().getSize() / glm::uvec2(2, 2);

	unsigned i = 0;
	for (PieceType type : pieceTypes)
	{
		auto res = m_piecePromotionBoxes.emplace(type, PiecePromotionBox(
			m_board.getTileSize(), scrMid + glm::ivec2{startX + 100 * i, -50},
			RenderedPiece::makePieceTexture(m_ownColor, type)
		));
		assert(res.second);

		i++;
	}

	m_ingameState.onMouseMoved          = bind(&RenderedMatch::onPromotionPieceMouseMotion, this, _1);
	m_ingameState.onMouseButtonReleased = bind(&RenderedMatch::onPromotionPieceClick, this, _1);
}
