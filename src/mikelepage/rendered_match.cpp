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
		, m_board(renderer, color)
		, m_gameEnded{false}
		, m_ownColor{color}
		, m_opColor{!color}
		, m_setupAccepted{false}
		, m_dragonTiles{{m_board.getTileSize(), m_board.getTileSize()}}
		, m_hoveringDragonTile{{false, false}}
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

		glm::uvec2 boardSize = m_board.getSize();
		glm::uvec2 boardPos = m_board.getPosition();
		glm::uvec2 tileSize = m_board.getTileSize();

		auto tmpTexture = makeTexture("setup-done.png");

		m_buttonSetupDoneTexture = std::move(tmpTexture.first);
		m_buttonSetupDone.setPosition(boardPos + boardSize - tmpTexture.second);
		m_buttonSetupDone.setSize(tmpTexture.second); // hardcoded for now, can be done properly somewhen else
		m_buttonSetupDone.setTexture(m_buttonSetupDoneTexture);

		m_dragonTiles[m_ownColor].setColor(Board::tileColors[1]);
		m_dragonTiles[m_ownColor].setPosition(
			{boardPos.x, boardPos.y + boardSize.y - tileSize.y}
		);

		m_dragonTiles[m_opColor].setColor(Board::tileColorsDark[1]);
		m_dragonTiles[m_opColor].setPosition(
			{boardPos.x + boardSize.x - tileSize.x, boardPos.y}
		);

		for(auto& quad : m_piecePromotionBackground)
			quad.setColor({95, 95, 95});


		placePiecesSetup();

		ingameState.tick                  = std::bind(&RenderedMatch::tick, this);
		ingameState.onMouseMoved          = std::bind(&Board::onMouseMoved, &m_board, _1);
		ingameState.onMouseButtonPressed  = std::bind(&Board::onMouseButtonPressed, &m_board, _1);
		ingameState.onMouseButtonReleased = std::bind(&Board::onMouseButtonReleased, &m_board, _1);
		ingameState.onKeyPressed          = [](const fea::Event::KeyEvent&) { };
		ingameState.onKeyReleased         = [](const fea::Event::KeyEvent&) { };

		m_board.onTileClicked      = std::bind(&RenderedMatch::onTileClicked, this, _1);
		m_board.onMouseMoveOutside = std::bind(&RenderedMatch::onMouseMoveOutsideBoard, this, _1);
		m_board.onClickedOutside   = std::bind(&RenderedMatch::onClickedOutsideBoard, this, _1);

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
		m_board.tick();

		for(auto player : m_players)
			if(player->dragonAliveInactive())
				m_renderer.queue(m_dragonTiles[player->getColor()]);

		for(fea::Quad* it : m_terrainToRender)
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

	void RenderedMatch::onTileClicked(Coordinate coord)
	{
		// if the player left the setup but the opponent isn't ready yet
		if(m_setup == m_setupAccepted)
			return;

		if(!m_setup && m_activePlayer != m_ownColor)
			return;

		if(!m_selectedPiece)
		{
			// search for a piece on the clicked tile
			auto it = m_activePieces.find(coord);

			if(it != m_activePieces.end() && it->second->getColor() == m_ownColor)
			{
				// a piece of the player was clicked
				m_selectedPiece = it->second;

				m_board.highlightTile(coord, "red", m_setup);
				if(!m_setup)
					showPossibleTargetTiles();
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
					m_selectedPiece.reset();
			}
			else
			{
				// there is a piece of the player on the clicked tile
				// => move the focus to the clicked tile

				assert(piece->getColor() == m_ownColor);

				resetSelectedTile();
				clearPossibleTargetTiles();

				m_selectedPiece = piece;

				m_board.highlightTile(coord, "red", m_setup);
				if(!m_setup)
					showPossibleTargetTiles();
			}
		}
	}

	void RenderedMatch::onMouseMoveOutsideBoard(const fea::Event::MouseMoveEvent& event)
	{
		for(auto player : m_players)
		{
			PlayersColor color = player->getColor();

			if(player->dragonAliveInactive() &&
			   mouseOver(m_dragonTiles[color], {event.x, event.y}))
			{
				if(!m_hoveringDragonTile[color])
				{
					m_dragonTiles[color].setColor(m_dragonTiles[color].getColor() + Board::hoverColor);
					m_hoveringDragonTile[color] = true;
				}
			}
			else if(m_hoveringDragonTile[color])
			{
				m_dragonTiles[color].setColor(m_dragonTiles[color].getColor() - Board::hoverColor);
				m_hoveringDragonTile[color] = false;
			}
		}
	}

	void RenderedMatch::onClickedOutsideBoard(const fea::Event::MouseButtonEvent& event)
	{
		// a piece is selected
		if(m_selectedPiece)
		{
			resetSelectedTile();
			clearPossibleTargetTiles();

			m_selectedPiece.reset();
		}

		// 'Setup done' button clicked (while being visible)
		if(m_self->setupComplete() && !m_setupAccepted && mouseOver(m_buttonSetupDone, {event.x, event.y}))
		{
			m_setupAccepted = true;
			// send before modifying m_activePieces, so the map doesn't
			// have to be filtered for only black / white pieces
			m_self->sendLeaveSetup();
			tryLeaveSetup();
		}
		else if(m_self->dragonAliveInactive() &&
			   mouseOver(m_dragonTiles[m_ownColor], {event.x, event.y}) &&
			   (m_setup || m_activePlayer == m_ownColor))
		{
			assert(m_self->getInactivePieces().count(PieceType::DRAGON) == 1);
			auto it = m_self->getInactivePieces().find(PieceType::DRAGON);

			m_selectedPiece = it->second;

			Board::highlight(m_dragonTiles[m_ownColor], Board::tileColors[1], "red");

			if(!m_setup)
				showPossibleTargetTiles();
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

			m_ingameState.onMouseMoved          = std::bind(&Board::onMouseMoved, &m_board, _1);
			m_ingameState.onMouseButtonPressed  = std::bind(&Board::onMouseButtonPressed, &m_board, _1);
			m_ingameState.onMouseButtonReleased = std::bind(&Board::onMouseButtonReleased, &m_board, _1);
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
			m_board.onTileClicked = std::bind(&RenderedMatch::onTileClicked, this, _1);
		}
	}

	void RenderedMatch::placePiece(std::shared_ptr<RenderedPiece> piece, std::shared_ptr<Player> player)
	{
		PlayersColor color = player->getColor();

		if(piece->getCoord()) // not null - a piece on the board
		{
			Coordinate coord = *piece->getCoord();

			m_activePieces.emplace(coord, piece);

			if(piece->getType() == PieceType::KING)
			{
				auto fortress = std::make_shared<RenderedFortress>(color, coord, m_board);

				player->setFortress(fortress);
				m_terrainToRender.push_back(fortress->getQuad());
			}
			else
			{
				TerrainType tType = piece->getSetupTerrain();
				if(tType != TerrainType::UNDEFINED)
				{
					auto terrain = std::make_shared<RenderedTerrain>(tType, coord, m_board, m_terrain);

					m_terrain.emplace(coord, terrain);
					m_terrainToRender.push_back(terrain->getQuad());
				}
			}
		}
		else // null - dragon outside the board
		{
			assert(piece->getType() == PieceType::DRAGON);
			assert(player->getInactivePieces().empty());

			piece->setPosition(m_dragonTiles[color].getPosition());
			player->getInactivePieces().emplace(piece->getType(), piece);
		}

		m_piecesToRender.push_back(piece->getQuad());
	}

	void RenderedMatch::placePiecesSetup()
	{
		#define coord(x, y) \
			Coordinate::create((x), (y))

		typedef std::pair<PieceType, std::shared_ptr<Coordinate>> Position;

		static const std::array<std::vector<Position>, 2> defaultPiecePositions {{
			{
				// white
				{PieceType::MOUNTAIN,    coord(0, 10)},
				{PieceType::MOUNTAIN,    coord(1, 10)},
				{PieceType::MOUNTAIN,    coord(2, 10)},
				{PieceType::MOUNTAIN,    coord(3, 10)},
				{PieceType::MOUNTAIN,    coord(4, 10)},
				{PieceType::MOUNTAIN,    coord(5, 10)},

				{PieceType::TREBUCHET,   coord(1, 8)},
				{PieceType::TREBUCHET,   coord(2, 8)},
				{PieceType::ELEPHANT,    coord(3, 8)},
				{PieceType::ELEPHANT,    coord(4, 8)},
				{PieceType::HEAVY_HORSE, coord(5, 8)},
				{PieceType::HEAVY_HORSE, coord(6, 8)},

				{PieceType::RABBLE,      coord(1, 7)},
				{PieceType::RABBLE,      coord(2, 7)},
				{PieceType::RABBLE,      coord(3, 7)},
				{PieceType::KING,        coord(4, 7)},
				{PieceType::RABBLE,      coord(5, 7)},
				{PieceType::RABBLE,      coord(6, 7)},
				{PieceType::RABBLE,      coord(7, 7)},

				{PieceType::CROSSBOWS,   coord(2, 6)},
				{PieceType::CROSSBOWS,   coord(3, 6)},
				{PieceType::SPEARS,      coord(4, 6)},
				{PieceType::SPEARS,      coord(5, 6)},
				{PieceType::LIGHT_HORSE, coord(6, 6)},
				{PieceType::LIGHT_HORSE, coord(7, 6)},

				// dragon starts outside the board
				{PieceType::DRAGON,      nullptr}
			},
			{
				// black
				{PieceType::MOUNTAIN,    coord(10, 0)},
				{PieceType::MOUNTAIN,    coord(9,  0)},
				{PieceType::MOUNTAIN,    coord(8,  0)},
				{PieceType::MOUNTAIN,    coord(7,  0)},
				{PieceType::MOUNTAIN,    coord(6,  0)},
				{PieceType::MOUNTAIN,    coord(5,  0)},

				{PieceType::TREBUCHET,   coord(9, 2)},
				{PieceType::TREBUCHET,   coord(8, 2)},
				{PieceType::ELEPHANT,    coord(7, 2)},
				{PieceType::ELEPHANT,    coord(6, 2)},
				{PieceType::HEAVY_HORSE, coord(5, 2)},
				{PieceType::HEAVY_HORSE, coord(4, 2)},

				{PieceType::RABBLE,      coord(9, 3)},
				{PieceType::RABBLE,      coord(8, 3)},
				{PieceType::RABBLE,      coord(7, 3)},
				{PieceType::KING,        coord(6, 3)},
				{PieceType::RABBLE,      coord(5, 3)},
				{PieceType::RABBLE,      coord(4, 3)},
				{PieceType::RABBLE,      coord(3, 3)},

				{PieceType::CROSSBOWS,   coord(8, 4)},
				{PieceType::CROSSBOWS,   coord(7, 4)},
				{PieceType::SPEARS,      coord(6, 4)},
				{PieceType::SPEARS,      coord(5, 4)},
				{PieceType::LIGHT_HORSE, coord(4, 4)},
				{PieceType::LIGHT_HORSE, coord(3, 4)},

				// dragon starts outside the board
				{PieceType::DRAGON,      nullptr}
			}}};

		#undef coord

		for(auto& it : defaultPiecePositions.at(m_ownColor))
		{
			auto tmpPiece = std::make_shared<RenderedPiece>(
				it.first, make_unique(it.second), m_ownColor, *this
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

		// temporary variable to hold the dragon piece if it isn't
		// "brought out" to the board (else this variable will remain unchanged)
		std::shared_ptr<Piece> unpositionedDragon;

		// TODO: rewrite the following stuff when adding a bot
		std::shared_ptr<RemotePlayer> op = std::dynamic_pointer_cast<RemotePlayer>(m_op);
		assert(op);

		for(auto piece : op->getPieceCache())
			placePiece(piece, op);

		op->clearPieceCache();

		m_bearingTable.init();

		if(m_ownColor == PlayersColor::WHITE)
			m_board.resetTileColors(5, 11);
		else
			m_board.resetTileColors(0, 5);

		m_dragonTiles[m_opColor].setColor(Board::tileColors[1]);

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
					m_self->sendMovePiece(piece, make_unique(oldCoord));

				if(!m_setupAccepted)
					m_self->checkSetupComplete();
			}

			clearPossibleTargetTiles();
			if(oldCoord)
				m_board.resetTileColor(*oldCoord, m_setup);

			if(!m_setup)
			{
				m_players[m_activePlayer]->onTurnEnd();

				m_activePlayer = !m_activePlayer;

				// ugly hack [TODO]
				if(m_activePlayer == m_opColor || getStatus() != "Select fortress replacement corner")
					updateTurnStatus();

				if(m_activePlayer == m_ownColor)
					m_self->onTurnBegin();
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

		rPiece->setPosition(m_board.getTileAt(coord)->getPosition());

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

			m_fortressReplacementTileHighlightings.push_back(std::make_shared<fea::Quad>(m_board.getTileSize()));
			m_fortressReplacementTileHighlightings.back()->setColor({127, 0, 191, 127});
			m_fortressReplacementTileHighlightings.back()->setPosition(m_board.getTilePosition(coord));

			m_terrainToRender.push_back(m_fortressReplacementTileHighlightings.back().get());

			m_bearingTable.update();

			return true;
		}

		return false;
	}

	void RenderedMatch::removeTerrain(fea::Quad* quad)
	{
		auto it = std::find(m_terrainToRender.begin(), m_terrainToRender.end(), quad);
		assert(it != m_terrainToRender.end());
		m_terrainToRender.erase(it);
	}

	void RenderedMatch::updateTurnStatus()
	{
		std::string status = PlayersColorToStr(m_activePlayer) + " players turn";
		status[0] -= ('a' - 'A'); // lowercase to uppercase

		setStatus(status);
	}

	void RenderedMatch::resetSelectedTile()
	{
		assert(m_selectedPiece);

		if(m_selectedPiece->getCoord())
		{
			m_board.resetTileColor(*m_selectedPiece->getCoord(), m_setup);
		}
		else
		{
			assert(m_selectedPiece->getType() == PieceType::DRAGON);

			if(m_selectedPiece->getColor() == m_ownColor)
				m_dragonTiles[m_ownColor].setColor(Board::tileColors[1]);
			else
				m_dragonTiles[m_opColor].setColor(m_setup ? Board::tileColorsDark[1] : Board::tileColors[1]);
		}
	}

	void RenderedMatch::showPossibleTargetTiles()
	{
		assert(!m_setup);
		assert(m_selectedPiece);

		// the tile clicked on holds a piece of the player
		for(auto targetTile : m_selectedPiece->getPossibleTargetTiles())
		{
			m_board.highlightTile(targetTile, "blue", false);

			m_possibleTargetTiles.emplace(targetTile);
		}
	}

	void RenderedMatch::clearPossibleTargetTiles()
	{
		for(auto it : m_possibleTargetTiles)
			m_board.resetTileColor(it, m_setup);

		m_possibleTargetTiles.clear();
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
		setStatus("Select fortress replacement corner");
		m_board.onTileClicked = std::bind(&RenderedMatch::onTileClickedFortressReplaceSelect, this, _1);
	}

	void RenderedMatch::endGame(PlayersColor winner)
	{
		std::string status = PlayersColorToStr(winner) + " player won!";
		status[0] -= ('a' - 'A'); // lowercase to uppercase

		setStatus(status);

		m_ingameState.onMouseMoved          = [](const fea::Event::MouseMoveEvent&) { };
		m_ingameState.onMouseButtonPressed  = [](const fea::Event::MouseButtonEvent&) { };
		m_ingameState.onMouseButtonReleased = [](const fea::Event::MouseButtonEvent&) { };
		m_ingameState.onKeyPressed          = [](const fea::Event::KeyEvent&) { };
		m_ingameState.onKeyReleased         = [](const fea::Event::KeyEvent&) { };

		m_gameEnded = true;
	}
}
