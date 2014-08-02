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
		: _renderer{renderer}
		, _ingameState(ingameState)
		, _board(renderer, color)
		, _gameEnded{false}
		, _ownColor{color}
		, _opColor{!color}
		, _setupAccepted{false}
		, _dragonTiles{{_board.getTileSize(), _board.getTileSize()}}
		, _hoveringDragonTile{{false, false}}
		, _piecePromotionBackground{{glm::vec2{100, 100}, glm::vec2{100, 100}, glm::vec2{100, 100}}}
		, _piecePromotionTypes{{PieceType::UNDEFINED, PieceType::UNDEFINED, PieceType::UNDEFINED}}
		, _renderPiecePromotionBgs{0}
		, _piecePromotionHover{0}
		, _piecePromotionMousePress{0}
	{
		_players[_ownColor] = std::make_shared<LocalPlayer>(_ownColor, *this);
		_players[_opColor]  = std::make_shared<RemotePlayer>(_opColor, *this);

		_self = std::dynamic_pointer_cast<LocalPlayer>(_players[_ownColor]);
		_op   = _players[_opColor];

		assert(_self);

		glm::uvec2 boardSize = _board.getSize();
		glm::uvec2 boardPos = _board.getPosition();

		auto tmpTexture = makeTexture("setup-done.png");

		_buttonSetupDoneTexture = std::move(tmpTexture.first);
		_buttonSetupDone.setPosition(boardPos + boardSize - tmpTexture.second);
		_buttonSetupDone.setSize(tmpTexture.second); // hardcoded for now, can be done properly somewhen else
		_buttonSetupDone.setTexture(_buttonSetupDoneTexture);

		_dragonTiles[_ownColor].setColor(Board::tileColors[1]);
		_dragonTiles[_ownColor].setPosition(
			{boardPos.x, boardPos.y + boardSize.y - _board.getTileSize().y}
		);

		_dragonTiles[_opColor].setColor(Board::tileColorsDark[1]);
		_dragonTiles[_opColor].setPosition(
			{boardPos.x + boardSize.x - _board.getTileSize().x, boardPos.y}
		);

		for(auto& quad : _piecePromotionBackground)
			quad.setColor({95, 95, 95});


		placePiecesSetup();

		ingameState.tick                  = std::bind(&RenderedMatch::tick, this);
		ingameState.onMouseMoved          = std::bind(&Board::onMouseMoved, &_board, _1);
		ingameState.onMouseButtonPressed  = std::bind(&Board::onMouseButtonPressed, &_board, _1);
		ingameState.onMouseButtonReleased = std::bind(&Board::onMouseButtonReleased, &_board, _1);
		ingameState.onKeyPressed          = [](const fea::Event::KeyEvent&) { };
		ingameState.onKeyReleased         = [](const fea::Event::KeyEvent&) { };

		_board.onTileClicked      = std::bind(&RenderedMatch::onTileClicked, this, _1);
		_board.onMouseMoveOutside = std::bind(&RenderedMatch::onMouseMoveOutsideBoard, this, _1);
		_board.onClickedOutside   = std::bind(&RenderedMatch::onClickedOutsideBoard, this, _1);

		setStatus("Setup");
	}

	const std::string& RenderedMatch::getStatus()
	{
		return _status;
	}

	void RenderedMatch::setStatus(const std::string& text)
	{
		if(!_gameEnded)
		{
			_status = text;

			#ifdef EMSCRIPTEN
			EM_ASM_({
				Module.setStatus(Module.Pointer_stringify($0));
			}, text.c_str());
			#endif
		}
	}

	void RenderedMatch::tick()
	{
		_board.tick();

		for(auto player : _players)
			if(player->dragonAliveInactive())
				_renderer.queue(_dragonTiles[player->getColor()]);

		for(fea::Quad* it : _terrainToRender)
			_renderer.queue(*it);
		for(fea::Quad* it : _piecesToRender)
			_renderer.queue(*it);

		if(_setup && _self->setupComplete() && !_setupAccepted)
			_renderer.queue(_buttonSetupDone);
		else if(_renderPiecePromotionBgs > 0)
		{
			for(int i = 0; i < _renderPiecePromotionBgs; ++i)
			{
				_renderer.queue(_piecePromotionBackground[i]);
				_renderer.queue(*_piecePromotionPieces[i]);
			}
		}
	}

	void RenderedMatch::onTileClicked(Coordinate coord)
	{
		// if the player left the setup but the opponent isn't ready yet
		if(_setup == _setupAccepted)
			return;

		if(!_setup && _activePlayer != _ownColor)
			return;

		if(!_selectedPiece)
		{
			// search for a piece on the clicked tile
			auto it = _activePieces.find(coord);

			if(it != _activePieces.end() && it->second->getColor() == _ownColor)
			{
				// a piece of the player was clicked
				_selectedPiece = it->second;

				_board.highlightTile(coord, "red", _setup);
				if(!_setup)
					showPossibleTargetTiles();
			}
		}
		else // a piece is selected
		{
			// determine which piece is on the clicked tile
			std::shared_ptr<Piece> piece;

			auto pieceIt = _activePieces.find(coord);
			if(pieceIt != _activePieces.end())
				piece = pieceIt->second;

			if(!piece || piece->getColor() == _opColor)
			{
				if(tryMovePiece(_selectedPiece, coord))
					_selectedPiece.reset();
			}
			else
			{
				// there is a piece of the player on the clicked tile
				// => move the focus to the clicked tile

				assert(piece->getColor() == _ownColor);

				resetSelectedTile();
				clearPossibleTargetTiles();

				_selectedPiece = piece;

				_board.highlightTile(coord, "red", _setup);
				if(!_setup)
					showPossibleTargetTiles();
			}
		}
	}

	void RenderedMatch::onMouseMoveOutsideBoard(const fea::Event::MouseMoveEvent& event)
	{
		for(auto player : _players)
		{
			PlayersColor color = player->getColor();

			if(player->dragonAliveInactive() &&
			   mouseOver(_dragonTiles[color], {event.x, event.y}))
			{
				if(!_hoveringDragonTile[color])
				{
					_dragonTiles[color].setColor(_dragonTiles[color].getColor() + Board::hoverColor);
					_hoveringDragonTile[color] = true;
				}
			}
			else if(_hoveringDragonTile[color])
			{
				_dragonTiles[color].setColor(_dragonTiles[color].getColor() - Board::hoverColor);
				_hoveringDragonTile[color] = false;
			}
		}
	}

	void RenderedMatch::onClickedOutsideBoard(const fea::Event::MouseButtonEvent& event)
	{
		// a piece is selected
		if(_selectedPiece)
		{
			resetSelectedTile();
			clearPossibleTargetTiles();

			_selectedPiece.reset();
		}

		// 'Setup done' button clicked (while being visible)
		if(_self->setupComplete() && !_setupAccepted && mouseOver(_buttonSetupDone, {event.x, event.y}))
		{
			_setupAccepted = true;
			// send before modifying _activePieces, so the map doesn't
			// have to be filtered for only black / white pieces
			_self->sendLeaveSetup();
			tryLeaveSetup();
		}
		else if(_self->dragonAliveInactive() &&
			   mouseOver(_dragonTiles[_ownColor], {event.x, event.y}) &&
			   (_setup || _activePlayer == _ownColor))
		{
			assert(_self->getInactivePieces().count(PieceType::DRAGON) == 1);
			auto it = _self->getInactivePieces().find(PieceType::DRAGON);

			_selectedPiece = it->second;

			Board::highlight(_dragonTiles[_ownColor], Board::tileColors[1], "red");

			if(!_setup)
				showPossibleTargetTiles();
		}
	}

	void RenderedMatch::onMouseMovedPromotionPieceSelect(const fea::Event::MouseMoveEvent& mouseMove)
	{
		bool hoverOne = false;

		for(int i = 0; i < _renderPiecePromotionBgs; i++)
			if(mouseOver(_piecePromotionBackground[i], {mouseMove.x, mouseMove.y}))
			{
				hoverOne = true;

				_piecePromotionHover = i+1;
				_piecePromotionBackground[i].setColor({127, 127, 127});
			}
			else
			{
				_piecePromotionBackground[i].setColor({95, 95, 95});
			}

		if(!hoverOne)
			_piecePromotionHover = 0;
	}

	void RenderedMatch::onMouseButtonPressedPromotionPieceSelect(const fea::Event::MouseButtonEvent& mouseButton)
	{
		if(mouseButton.button != fea::Mouse::Button::LEFT || _piecePromotionMousePress)
			return;

		if(_piecePromotionHover)
			_piecePromotionMousePress = _piecePromotionHover;
	}

	void RenderedMatch::onMouseButtonReleasedPromotionPieceSelect(const fea::Event::MouseButtonEvent& mouseButton)
	{
		if(mouseButton.button != fea::Mouse::Button::LEFT || !_piecePromotionMousePress)
			return;

		if(_piecePromotionMousePress == _piecePromotionHover)
		{
			assert(_self->getFortress());

			auto piece = getPieceAt(_self->getFortress()->getCoord());
			assert(piece);

			PieceType oldType = piece->getType();
			PieceType newType = _piecePromotionTypes[_piecePromotionMousePress-1];

			piece->promoteTo(newType);
			_self->sendPromotePiece(oldType, newType);

			_renderPiecePromotionBgs = 0;
			_piecePromotionPieces.fill(nullptr);

			_ingameState.onMouseMoved          = std::bind(&Board::onMouseMoved, &_board, _1);
			_ingameState.onMouseButtonPressed  = std::bind(&Board::onMouseButtonPressed, &_board, _1);
			_ingameState.onMouseButtonReleased = std::bind(&Board::onMouseButtonReleased, &_board, _1);
		}

		_piecePromotionMousePress = 0;
	}

	void RenderedMatch::onTileClickedFortressReplaceSelect(Coordinate coord)
	{
		if(addFortressReplacementTile(coord))
		{
			// another ugly hack... pure laziness
			setStatus("");
			updateTurnStatus();

			_self->sendAddFortressReplacementTile(coord);
			_board.onTileClicked = std::bind(&RenderedMatch::onTileClicked, this, _1);
		}
	}

	void RenderedMatch::placePiece(std::shared_ptr<RenderedPiece> piece, std::shared_ptr<Player> player)
	{
		PlayersColor color = player->getColor();

		if(piece->getCoord()) // not null - a piece on the board
		{
			Coordinate coord = *piece->getCoord();

			_activePieces.emplace(coord, piece);

			if(piece->getType() == PieceType::KING)
			{
				auto fortress = std::make_shared<RenderedFortress>(color, coord, _board);

				player->setFortress(fortress);
				_terrainToRender.push_back(fortress->getQuad());
			}
			else
			{
				TerrainType tType = piece->getSetupTerrain();
				if(tType != TerrainType::UNDEFINED)
				{
					auto terrain = std::make_shared<RenderedTerrain>(tType, coord, _board, _terrain);

					_terrain.emplace(coord, terrain);
					_terrainToRender.push_back(terrain->getQuad());
				}
			}
		}
		else // null - dragon outside the board
		{
			assert(piece->getType() == PieceType::DRAGON);
			assert(player->getInactivePieces().empty());

			piece->setPosition(_dragonTiles[color].getPosition());
			player->getInactivePieces().emplace(piece->getType(), piece);
		}

		_piecesToRender.push_back(piece->getQuad());
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

		for(auto& it : defaultPiecePositions.at(_ownColor))
		{
			auto tmpPiece = std::make_shared<RenderedPiece>(
				it.first, make_unique(it.second), _ownColor, *this
			);

			placePiece(tmpPiece, _self);
		}
	}

	void RenderedMatch::tryLeaveSetup()
	{
		if(!_setupAccepted) return;

		for(auto player : _players)
			if(!player->setupComplete())
				return;

		_setup = false;

		// temporary variable to hold the dragon piece if it isn't
		// "brought out" to the board (else this variable will remain unchanged)
		std::shared_ptr<Piece> unpositionedDragon;

		// TODO: rewrite the following stuff when adding a bot
		std::shared_ptr<RemotePlayer> op = std::dynamic_pointer_cast<RemotePlayer>(_op);
		assert(op);

		for(auto piece : op->getPieceCache())
			placePiece(piece, op);

		op->clearPieceCache();

		_bearingTable.init();

		if(_ownColor == PlayersColor::WHITE)
			_board.resetTileColors(5, 11);
		else
			_board.resetTileColors(0, 5);

		_dragonTiles[_opColor].setColor(Board::tileColors[1]);

		updateTurnStatus();
	}

	bool RenderedMatch::tryMovePiece(std::shared_ptr<Piece> piece, Coordinate coord)
	{
		assert(piece);

		auto oldCoord = piece->getCoord();

		if(piece->moveTo(coord, _setup))
		{
			// move is valid and was done

			if(piece->getColor() == _ownColor)
			{
				if(!_setup)
					_self->sendMovePiece(piece, make_unique(oldCoord));

				if(!_setupAccepted)
					_self->checkSetupComplete();
			}

			clearPossibleTargetTiles();
			if(oldCoord)
				_board.resetTileColor(*oldCoord, _setup);

			if(!_setup)
			{
				_players[_activePlayer]->onTurnEnd();

				_activePlayer = !_activePlayer;

				// ugly hack [TODO]
				if(_activePlayer == _opColor || getStatus() != "Select fortress replacement corner")
					updateTurnStatus();

				if(_activePlayer == _ownColor)
					_self->onTurnBegin();
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

		rPiece->setPosition(_board.getTileAt(coord)->getPosition());

		_piecesToRender.push_back(rPiece->getQuad());
	}

	void RenderedMatch::removeFromBoard(std::shared_ptr<Piece> piece)
	{
		Match::removeFromBoard(piece);

		// TODO: place the piece somewhere outside
		// the board instead of not rendering it

		auto rPiece = std::dynamic_pointer_cast<RenderedPiece>(piece);
		assert(rPiece);

		auto it = std::find(_piecesToRender.begin(), _piecesToRender.end(), rPiece->getQuad());
		assert(it != _piecesToRender.end());
		_piecesToRender.erase(it);

		PlayersColor pieceColor = piece->getColor();

		if(piece->getType() == PieceType::KING && !_players.at(pieceColor)->getFortress())
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
			_fortressReplaceCorners.insert(coord);

			_fortressReplacementTileHighlightings.push_back(std::make_shared<fea::Quad>(_board.getTileSize()));
			_fortressReplacementTileHighlightings.back()->setColor({127, 0, 191, 127});
			_fortressReplacementTileHighlightings.back()->setPosition(_board.getTilePosition(coord));

			_terrainToRender.push_back(_fortressReplacementTileHighlightings.back().get());

			_bearingTable.clear();
			_bearingTable.init();

			return true;
		}

		return false;
	}

	void RenderedMatch::removeTerrain(fea::Quad* quad)
	{
		auto it = std::find(_terrainToRender.begin(), _terrainToRender.end(), quad);
		assert(it != _terrainToRender.end());
		_terrainToRender.erase(it);
	}

	void RenderedMatch::updateTurnStatus()
	{
		std::string status = PlayersColorToStr(_activePlayer) + " players turn";
		status[0] -= ('a' - 'A'); // lowercase to uppercase

		setStatus(status);
	}

	void RenderedMatch::resetSelectedTile()
	{
		assert(_selectedPiece);

		if(_selectedPiece->getCoord())
		{
			_board.resetTileColor(*_selectedPiece->getCoord(), _setup);
		}
		else
		{
			assert(_selectedPiece->getType() == PieceType::DRAGON);

			if(_selectedPiece->getColor() == _ownColor)
				_dragonTiles[_ownColor].setColor(Board::tileColors[1]);
			else
				_dragonTiles[_opColor].setColor(_setup ? Board::tileColorsDark[1] : Board::tileColors[1]);
		}
	}

	void RenderedMatch::showPossibleTargetTiles()
	{
		assert(!_setup);
		assert(_selectedPiece);

		// the tile clicked on holds a piece of the player
		for(auto targetTile : _selectedPiece->getPossibleTargetTiles())
		{
			_board.highlightTile(targetTile, "blue", false);

			_possibleTargetTiles.emplace(targetTile);
		}
	}

	void RenderedMatch::clearPossibleTargetTiles()
	{
		for(auto it : _possibleTargetTiles)
			_board.resetTileColor(it, _setup);

		_possibleTargetTiles.clear();
	}

	void RenderedMatch::showPromotionPieces(std::set<PieceType> pieceTypes)
	{
		assert(pieceTypes.size() > 1 && pieceTypes.size() <= 3);

		_renderPiecePromotionBgs = pieceTypes.size();
		glm::uvec2 scrMid = _renderer.getViewport().getSize() / glm::uvec2{2, 2};

		if(pieceTypes.size() == 2)
		{
			_piecePromotionBackground[0].setPosition(glm::ivec2(scrMid) + glm::ivec2{-100, -50});
			_piecePromotionBackground[1].setPosition(glm::ivec2(scrMid) + glm::ivec2{0, -50});
		}
		else
		{
			_piecePromotionBackground[0].setPosition(glm::ivec2(scrMid) + glm::ivec2{-150, -50});
			_piecePromotionBackground[1].setPosition(glm::ivec2(scrMid) + glm::ivec2{-50, -50});
			_piecePromotionBackground[2].setPosition(glm::ivec2(scrMid) + glm::ivec2{50, -50});
		}

		auto& inactivePieces = _self->getInactivePieces();

		uint_least8_t i = 0;
		for(PieceType type : pieceTypes)
		{
			_piecePromotionTypes[i] = type;

			auto it = inactivePieces.find(type);
			assert(it != inactivePieces.end());

			auto rPiece = std::dynamic_pointer_cast<RenderedPiece>(it->second);
			assert(rPiece);

			rPiece->setPosition(_piecePromotionBackground[i].getPosition() + glm::vec2{17, 25}); // TODO
			_piecePromotionPieces[i] = rPiece->getQuad();
			i++;
		}

		_ingameState.onMouseMoved          = std::bind(&RenderedMatch::onMouseMovedPromotionPieceSelect, this, _1);
		_ingameState.onMouseButtonPressed  = std::bind(&RenderedMatch::onMouseButtonPressedPromotionPieceSelect, this, _1);
		_ingameState.onMouseButtonReleased = std::bind(&RenderedMatch::onMouseButtonReleasedPromotionPieceSelect, this, _1);
	}

	void RenderedMatch::chooseFortressReplacementTile()
	{
		setStatus("Select fortress replacement corner");
		_board.onTileClicked = std::bind(&RenderedMatch::onTileClickedFortressReplaceSelect, this, _1);
	}

	void RenderedMatch::endGame(PlayersColor winner)
	{
		std::string status = PlayersColorToStr(winner) + " player won!";
		status[0] -= ('a' - 'A'); // lowercase to uppercase

		setStatus(status);

		_ingameState.onMouseMoved          = [](const fea::Event::MouseMoveEvent&) { };
		_ingameState.onMouseButtonPressed  = [](const fea::Event::MouseButtonEvent&) { };
		_ingameState.onMouseButtonReleased = [](const fea::Event::MouseButtonEvent&) { };
		_ingameState.onKeyPressed          = [](const fea::Event::KeyEvent&) { };
		_ingameState.onKeyReleased         = [](const fea::Event::KeyEvent&) { };

		_gameEnded = true;
	}
}
