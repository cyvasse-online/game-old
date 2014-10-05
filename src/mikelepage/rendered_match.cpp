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

#include "rendered_match.hpp"

#include <algorithm>
#ifdef EMSCRIPTEN
	#include <emscripten.h>
#endif
#include "common.hpp"
#include "hexagon_board.hpp"
#include "ingame_state.hpp"
#include "local_player.hpp"
#include "rendered_fortress.hpp"
#include "rendered_piece.hpp"
#include "rendered_terrain.hpp"
#include "remote_player.hpp"
#include "texturemaker.hpp" // lodepng helper function

using namespace std::placeholders;
using namespace cyvmath::mikelepage;

namespace mikelepage
{
	RenderedMatch::RenderedMatch(IngameState& ingameState, fea::Renderer2D& renderer, PlayersColor color)
		: m_renderer{renderer}
		, m_ingameState(ingameState)
		, m_board{make_unique<Board>(renderer, color)}
		, m_gameEnded{false}
		, m_ownColor{color}
		, m_opColor{!color}
		, m_setupAccepted{false}
		, m_piecePromotionBackground{{glm::vec2{100, 100}, glm::vec2{100, 100}, glm::vec2{100, 100}}}
		, m_piecePromotionTypes{{PieceType::UNDEFINED, PieceType::UNDEFINED, PieceType::UNDEFINED}}
		, m_renderPiecePromotionBgs{0}
		, m_piecePromotionHover{0}
		, m_piecePromotionMousePress{0}
	{
		m_players[m_ownColor] = std::make_shared<LocalPlayer>(m_ownColor, *this);
		m_players[m_opColor]  = std::make_shared<RemotePlayer>(m_opColor, *this);

		m_self = std::dynamic_pointer_cast<LocalPlayer>(m_players[m_ownColor]);
		m_op   = m_players[m_opColor];

		assert(m_self);

		glm::uvec2 boardSize = m_board->getSize();
		glm::uvec2 boardPos = m_board->getPosition();

		auto tmpTexture = makeTexture("setup-done.png");

		m_buttonSetupDoneTexture = std::move(tmpTexture.first);
		m_buttonSetupDone.setPosition(boardPos + boardSize - tmpTexture.second);
		m_buttonSetupDone.setSize(tmpTexture.second); // hardcoded for now, can be done properly somewhen else
		m_buttonSetupDone.setTexture(m_buttonSetupDoneTexture);

		for(auto& quad : m_piecePromotionBackground)
			quad.setColor({95, 95, 95});

		placePiecesSetup();

		ingameState.tick                  = std::bind(&RenderedMatch::tick, this);
		ingameState.onMouseMoved          = std::bind(&Board::onMouseMoved, m_board.get(), _1);
		ingameState.onMouseButtonPressed  = std::bind(&Board::onMouseButtonPressed, m_board.get(), _1);
		ingameState.onMouseButtonReleased = std::bind(&Board::onMouseButtonReleased, m_board.get(), _1);
		ingameState.onKeyPressed          = [](const fea::Event::KeyEvent&) { };
		ingameState.onKeyReleased         = [](const fea::Event::KeyEvent&) { };

		m_board->onTileMouseOver    = std::bind(&RenderedMatch::onTileMouseOver, this, _1);
		m_board->onTileClicked      = std::bind(&RenderedMatch::onTileClicked, this, _1);
		m_board->onMouseMoveOutside = [](const fea::Event::MouseMoveEvent&) { };
		m_board->onClickedOutside   = std::bind(&RenderedMatch::onClickedOutsideBoard, this, _1);

		setStatus("Setup");
	}

	void RenderedMatch::setStatus(const std::string& text)
	{
		if(!m_gameEnded)
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
		m_board->queueTileRendering();

		for(fea::Quad* it : m_terrainToRender)
			m_renderer.queue(*it);

		m_board->queueHighlightingRendering();

		for(fea::Quad* it : m_fortressesToRender)
			m_renderer.queue(*it);
		for(fea::Quad* it : m_piecesToRender)
			m_renderer.queue(*it);

		if(m_setup && m_self->setupComplete() && !m_setupAccepted)
			m_renderer.queue(m_buttonSetupDone);
		else if(m_renderPiecePromotionBgs > 0)
		{
			for(int i = 0; i < m_renderPiecePromotionBgs; ++i)
			{
				m_renderer.queue(m_piecePromotionBackground[i]);
				m_renderer.queue(*m_piecePromotionPieces[i]);
			}
		}
	}

	void RenderedMatch::onTileMouseOver(Coordinate coord)
	{
		// if the player left the setup but the opponent isn't ready yet
		if(m_setup || !m_setupAccepted)
			return;

		if(m_selectedPiece) // a piece is selected
			return;

		// search for a piece on the clicked tile
		auto it = m_activePieces.find(coord);

		if(it != m_activePieces.end() &&
		   it->second->getType() != PieceType::MOUNTAINS)
		{
			// a piece of the player was hovered

			if(it->second != m_hoveredPiece)
			{
				m_hoveredPiece = it->second;

				showPossibleTargetTiles();
			}
		}
		else if(m_hoveredPiece)
		{
			m_hoveredPiece.reset();
			m_board->clearHighlighting(HighlightingId::PTT);
		}
	}

	void RenderedMatch::onTileClicked(Coordinate coord)
	{
		if(m_setup == m_setupAccepted)
			return;

		if(!m_setup && m_activePlayer != m_ownColor)
			return;

		if(!m_selectedPiece)
		{
			// search for a piece on the clicked tile
			auto it = m_activePieces.find(coord);

			if(it != m_activePieces.end() &&
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
			std::shared_ptr<Piece> piece;

			auto pieceIt = m_activePieces.find(coord);
			if(pieceIt != m_activePieces.end())
				piece = pieceIt->second;

			if(!piece || piece->getColor() == m_opColor)
			{
				if(tryMovePiece(m_selectedPiece, coord))
				{
					m_selectedPiece.reset();
					m_board->clearHighlighting(HighlightingId::SEL);

					if(!m_setup)
						showPossibleTargetTiles();
				}
			}
			else if(m_setup || piece->getType() != PieceType::MOUNTAINS)
			{
				// there is a piece of the player on the clicked tile
				auto selectedCoord = m_selectedPiece->getCoord();
				assert(selectedCoord);

				m_selectedPiece.reset();
				m_board->clearHighlighting(HighlightingId::SEL);

				if(coord != *selectedCoord)
				{
					// another piece clicked, create a new highlightning
					m_selectedPiece = piece;
					m_hoveredPiece = piece;

					m_board->highlightTile(coord, HighlightingId::SEL);
					if(!m_setup)
						showPossibleTargetTiles();
				}
			}
		}
	}

	void RenderedMatch::onClickedOutsideBoard(const fea::Event::MouseButtonEvent& event)
	{
		// 'Setup done' button clicked (while being visible)
		if(m_self->setupComplete() && !m_setupAccepted && mouseOver(m_buttonSetupDone, {event.x, event.y}))
		{
			m_setupAccepted = true;
			// send before modifying m_activePieces, so the map doesn't
			// have to be filtered for only black / white pieces
			m_self->sendLeaveSetup();
			tryLeaveSetup();
		}

		if(m_selectedPiece)
		{
			m_selectedPiece.reset();
			m_board->clearHighlighting(HighlightingId::SEL);
			m_board->clearHighlighting(HighlightingId::PTT);
		}
	}

	void RenderedMatch::onMouseMovedPromotionPieceSelect(const fea::Event::MouseMoveEvent& mouseMove)
	{
		bool hoverOne = false;

		for(int i = 0; i < m_renderPiecePromotionBgs; i++)
			if(mouseOver(m_piecePromotionBackground[i], {mouseMove.x, mouseMove.y}))
			{
				hoverOne = true;

				m_piecePromotionHover = i+1;
				m_piecePromotionBackground[i].setColor({127, 127, 127});
			}
			else
			{
				m_piecePromotionBackground[i].setColor({95, 95, 95});
			}

		if(!hoverOne)
			m_piecePromotionHover = 0;
	}

	void RenderedMatch::onMouseButtonPressedPromotionPieceSelect(const fea::Event::MouseButtonEvent& mouseButton)
	{
		if(mouseButton.button != fea::Mouse::Button::LEFT || m_piecePromotionMousePress)
			return;

		if(m_piecePromotionHover)
			m_piecePromotionMousePress = m_piecePromotionHover;
	}

	void RenderedMatch::onMouseButtonReleasedPromotionPieceSelect(const fea::Event::MouseButtonEvent& mouseButton)
	{
		if(mouseButton.button != fea::Mouse::Button::LEFT || !m_piecePromotionMousePress)
			return;

		if(m_piecePromotionMousePress == m_piecePromotionHover)
		{
			assert(m_self->getFortress());

			auto piece = getPieceAt(m_self->getFortress()->getCoord());
			assert(piece);

			PieceType oldType = piece->getType();
			PieceType newType = m_piecePromotionTypes[m_piecePromotionMousePress-1];

			piece->promoteTo(newType);
			m_self->sendPromotePiece(oldType, newType);

			m_renderPiecePromotionBgs = 0;
			m_piecePromotionPieces.fill(nullptr);

			m_ingameState.onMouseMoved          = std::bind(&Board::onMouseMoved, m_board.get(), _1);
			m_ingameState.onMouseButtonPressed  = std::bind(&Board::onMouseButtonPressed, m_board.get(), _1);
			m_ingameState.onMouseButtonReleased = std::bind(&Board::onMouseButtonReleased, m_board.get(), _1);
		}

		m_piecePromotionMousePress = 0;
	}

	void RenderedMatch::onTileClickedFortressReplaceSelect(Coordinate coord)
	{
		if(addFortressReplacementTile(coord))
		{
			// another ugly hack... pure laziness
			setStatus("");
			updateTurnStatus();

			m_self->sendAddFortressReplacementTile(coord);
			m_board->onTileMouseOver = std::bind(&RenderedMatch::onTileMouseOver, this, _1);
			m_board->onTileClicked   = std::bind(&RenderedMatch::onTileClicked, this, _1);
		}
	}

	void RenderedMatch::placePiece(std::shared_ptr<RenderedPiece> piece, std::shared_ptr<Player> player)
	{
		auto color = player->getColor();
		auto coord = piece->getCoord();
		auto pieceType = piece->getType();

		assert(coord);

		m_activePieces.emplace(*coord, piece);

		if(pieceType == PieceType::KING)
		{
			auto fortress = std::make_shared<RenderedFortress>(color, *coord, *m_board);

			player->setFortress(fortress);
			m_fortressesToRender.push_back(fortress->getQuad());
		}
		else
		{
			TerrainType tType = piece->getSetupTerrain();

			if(tType != TerrainType::UNDEFINED)
			{
				auto terrain = std::make_shared<RenderedTerrain>(tType, *coord, *m_board, m_terrain);

				m_terrain.emplace(*coord, terrain);
				m_terrainToRender.push_back(terrain->getQuad());
			}
		}

		m_piecesToRender.push_back(piece->getQuad());
	}

	void RenderedMatch::placePiecesSetup()
	{
		typedef std::pair<PieceType, Coordinate> Position;

		auto C = [](int_least8_t x, int_least8_t y) {
			auto coord = Coordinate::create(x, y);
			assert(coord);
			return *coord;
		};

		static const std::array<std::vector<Position>, 2> defaultPiecePositions {{
			{
				// white
				{PieceType::DRAGON,      C(3, 9)},

				{PieceType::MOUNTAINS,   C(0, 8)},
				{PieceType::MOUNTAINS,   C(0, 7)},
				{PieceType::MOUNTAINS,   C(1, 6)},
				{PieceType::MOUNTAINS,   C(7, 8)},
				{PieceType::MOUNTAINS,   C(8, 7)},
				{PieceType::MOUNTAINS,   C(8, 6)},

				{PieceType::TREBUCHET,   C(1, 8)},
				{PieceType::TREBUCHET,   C(2, 8)},
				{PieceType::ELEPHANT,    C(3, 8)},
				{PieceType::ELEPHANT,    C(4, 8)},
				{PieceType::HEAVY_HORSE, C(5, 8)},
				{PieceType::HEAVY_HORSE, C(6, 8)},

				{PieceType::CROSSBOWS,   C(1, 7)},
				{PieceType::CROSSBOWS,   C(2, 7)},
				{PieceType::SPEARS,      C(3, 7)},
				{PieceType::KING,        C(4, 7)},
				{PieceType::SPEARS,      C(5, 7)},
				{PieceType::LIGHT_HORSE, C(6, 7)},
				{PieceType::LIGHT_HORSE, C(7, 7)},

				{PieceType::RABBLE,      C(2, 6)},
				{PieceType::RABBLE,      C(3, 6)},
				{PieceType::RABBLE,      C(4, 6)},
				{PieceType::RABBLE,      C(5, 6)},
				{PieceType::RABBLE,      C(6, 6)},
				{PieceType::RABBLE,      C(7, 6)}
			},
			{
				// black
				{PieceType::DRAGON,      C(7, 1)},

				{PieceType::MOUNTAINS,   C(10, 2)},
				{PieceType::MOUNTAINS,   C(10, 3)},
				{PieceType::MOUNTAINS,   C(9, 4)},
				{PieceType::MOUNTAINS,   C(3, 2)},
				{PieceType::MOUNTAINS,   C(2, 3)},
				{PieceType::MOUNTAINS,   C(2, 4)},

				{PieceType::TREBUCHET,   C(9, 2)},
				{PieceType::TREBUCHET,   C(8, 2)},
				{PieceType::ELEPHANT,    C(7, 2)},
				{PieceType::ELEPHANT,    C(6, 2)},
				{PieceType::HEAVY_HORSE, C(5, 2)},
				{PieceType::HEAVY_HORSE, C(4, 2)},

				{PieceType::CROSSBOWS,   C(9, 3)},
				{PieceType::CROSSBOWS,   C(8, 3)},
				{PieceType::SPEARS,      C(7, 3)},
				{PieceType::KING,        C(6, 3)},
				{PieceType::SPEARS,      C(5, 3)},
				{PieceType::LIGHT_HORSE, C(4, 3)},
				{PieceType::LIGHT_HORSE, C(3, 3)},

				{PieceType::RABBLE,      C(8, 4)},
				{PieceType::RABBLE,      C(7, 4)},
				{PieceType::RABBLE,      C(6, 4)},
				{PieceType::RABBLE,      C(5, 4)},
				{PieceType::RABBLE,      C(4, 4)},
				{PieceType::RABBLE,      C(3, 4)}
			}}};

		for(auto& it : defaultPiecePositions.at(m_ownColor))
		{
			auto tmpPiece = std::make_shared<RenderedPiece>(
				it.first, it.second, m_ownColor, *this
			);

			placePiece(tmpPiece, m_self);
		}
	}

	void RenderedMatch::tryLeaveSetup()
	{
		if(!m_setupAccepted) return;

		for(auto player : m_players)
			if(!player->setupComplete())
				return;

		m_setup = false;

		// TODO: rewrite the following stuff when adding a bot
		std::shared_ptr<RemotePlayer> op = std::dynamic_pointer_cast<RemotePlayer>(m_op);
		assert(op);

		for(auto piece : op->getPieceCache())
			placePiece(piece, op);

		op->clearPieceCache();

		m_bearingTable.init();

		m_board->clearHighlighting(HighlightingId::DIM);

		updateTurnStatus();
	}

	bool RenderedMatch::tryMovePiece(std::shared_ptr<Piece> piece, Coordinate coord)
	{
		assert(piece);

		auto oldCoord = piece->getCoord();

		if(piece->moveTo(coord, m_setup))
		{
			// move is valid and was done

			if(piece->getColor() == m_ownColor)
			{
				if(!m_setup)
					m_self->sendMovePiece(piece, *oldCoord);

				if(!m_setupAccepted)
					m_self->checkSetupComplete();
			}

			m_board->clearHighlighting(HighlightingId::PTT);

			if(!m_setup)
			{
				m_players[m_activePlayer]->onTurnEnd();

				m_activePlayer = !m_activePlayer;

				// ugly hack [TODO]
				if(m_activePlayer == m_opColor || getStatus() != "Select fortress replacement corner")
					updateTurnStatus();

				if(m_activePlayer == m_ownColor)
					m_self->onTurnBegin();

				m_board->highlightTile(coord, HighlightingId::LAST_MOVE, true);
				m_board->highlightTile(*oldCoord, HighlightingId::LAST_MOVE, false);
			}

			return true;
		}

		return false;
	}

	void RenderedMatch::addToBoard(PieceType type, PlayersColor color, Coordinate coord)
	{
		Match::addToBoard(type, color, coord);

		auto rPiece = std::dynamic_pointer_cast<RenderedPiece>(getPieceAt(coord));
		assert(rPiece);

		rPiece->setPosition(m_board->getTileAt(coord)->getPosition());

		m_piecesToRender.push_back(rPiece->getQuad());
	}

	void RenderedMatch::removeFromBoard(std::shared_ptr<Piece> piece)
	{
		Match::removeFromBoard(piece);

		// TODO: place the piece somewhere outside
		// the board instead of not rendering it

		auto rPiece = std::dynamic_pointer_cast<RenderedPiece>(piece);
		assert(rPiece);

		auto it = std::find(m_piecesToRender.begin(), m_piecesToRender.end(), rPiece->getQuad());
		assert(it != m_piecesToRender.end());
		m_piecesToRender.erase(it);

		PlayersColor pieceColor = piece->getColor();

		if(piece->getType() == PieceType::KING && !m_players.at(pieceColor)->getFortress())
			endGame(!pieceColor);
	}

	bool RenderedMatch::addFortressReplacementTile(Coordinate coord)
	{
		static const std::set<Coordinate> cornerTiles {
			*Coordinate::create( 0, 10),
			*Coordinate::create( 5, 10),
			*Coordinate::create(10,  5),
			*Coordinate::create(10,  0),
			*Coordinate::create( 5,  0),
			*Coordinate::create( 0,  5)
		};

		if(cornerTiles.find(coord) != cornerTiles.end())
		{
			m_fortressReplaceCorners.insert(coord);

			m_fortressReplacementTileHighlightings.push_back(std::make_shared<fea::Quad>(m_board->getTileSize()));
			m_fortressReplacementTileHighlightings.back()->setColor({127, 0, 191, 127});
			m_fortressReplacementTileHighlightings.back()->setPosition(m_board->getTilePosition(coord));

			m_terrainToRender.push_back(m_fortressReplacementTileHighlightings.back().get());

			m_bearingTable.update();

			return true;
		}

		return false;
	}

	void RenderedMatch::removeFortress(fea::Quad* quad)
	{
		auto it = std::find(m_fortressesToRender.begin(), m_fortressesToRender.end(), quad);
		assert(it != m_fortressesToRender.end());
		m_fortressesToRender.erase(it);
	}

	void RenderedMatch::updateTurnStatus()
	{
		std::string status = PlayersColorToStr(m_activePlayer) + " player's turn";
		status[0] -= ('a' - 'A'); // lowercase to uppercase

		setStatus(status);
	}

	void RenderedMatch::showPossibleTargetTiles()
	{
		assert(!m_setup);

		if(!m_hoveredPiece)
			return;

		// should this be removed?
		// if not, merge with the above
		if(m_gameEnded)
			return;

		// the tile clicked on holds a piece of the player
		auto pTT = m_hoveredPiece->getPossibleTargetTiles();
		m_board->highlightTiles(pTT.begin(), pTT.end(), HighlightingId::PTT);
	}

	void RenderedMatch::showPromotionPieces(std::set<PieceType> pieceTypes)
	{
		assert(pieceTypes.size() > 1 && pieceTypes.size() <= 3);

		m_renderPiecePromotionBgs = pieceTypes.size();
		glm::uvec2 scrMid = m_renderer.getViewport().getSize() / glm::uvec2{2, 2};

		if(pieceTypes.size() == 2)
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

		auto& inactivePieces = m_self->getInactivePieces();

		uint_least8_t i = 0;
		for(PieceType type : pieceTypes)
		{
			m_piecePromotionTypes[i] = type;

			auto it = inactivePieces.find(type);
			assert(it != inactivePieces.end());

			auto rPiece = std::dynamic_pointer_cast<RenderedPiece>(it->second);
			assert(rPiece);

			rPiece->setPosition(m_piecePromotionBackground[i].getPosition() + glm::vec2{17, 25}); // TODO
			m_piecePromotionPieces[i] = rPiece->getQuad();
			i++;
		}

		m_ingameState.onMouseMoved          = std::bind(&RenderedMatch::onMouseMovedPromotionPieceSelect, this, _1);
		m_ingameState.onMouseButtonPressed  = std::bind(&RenderedMatch::onMouseButtonPressedPromotionPieceSelect, this, _1);
		m_ingameState.onMouseButtonReleased = std::bind(&RenderedMatch::onMouseButtonReleasedPromotionPieceSelect, this, _1);
	}

	void RenderedMatch::chooseFortressReplacementTile()
	{
		// TODO: Dim all tiles except the corners

		setStatus("Select fortress replacement corner");
		m_board->onTileMouseOver = [](Coordinate) { };
		m_board->onTileClicked   = std::bind(&RenderedMatch::onTileClickedFortressReplaceSelect, this, _1);
	}

	void RenderedMatch::endGame(PlayersColor winner)
	{
		std::string status = PlayersColorToStr(winner) + " player won!";
		status[0] -= ('a' - 'A'); // lowercase to uppercase

		setStatus(status);

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
