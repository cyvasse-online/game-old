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

#include "mikelepage_rule_set.hpp"

#include "common.hpp"
#include "remote_player.hpp"
// lodepng helper function
#include "texturemaker.hpp"

using namespace cyvmath::mikelepage;

namespace mikelepage
{
	MikelepageRuleSet::MikelepageRuleSet(IngameState& ingameState, fea::Renderer2D& renderer, PlayersColor color)
		: Match(color)
		, _renderer(renderer)
		, _board(renderer, color)
		, _selectedTile(Board::noTile())
	{
		PlayersColor opColor = (color == PLAYER_WHITE ? PLAYER_BLACK : PLAYER_WHITE);

		_players.emplace(color, std::make_shared<LocalPlayer>(color));
		_players.emplace(opColor, std::make_shared<RemotePlayer>(opColor));
		_self = std::dynamic_pointer_cast<LocalPlayer>(_players[color]);
		assert(_self);

		placePiecesSetup();

		glm::uvec2 boardSize = _board.getSize();
		glm::uvec2 boardPos = _board.getPosition();
		// hardcoded for now, can be done properly somewhen else
		glm::uvec2 buttonSize = {80, 50};

		_buttonSetupDoneTexture = makeTexture("setup-done.png", buttonSize.x, buttonSize.y);
		_buttonSetupDone.setPosition(boardPos + boardSize - buttonSize);
		_buttonSetupDone.setSize(buttonSize);
		_buttonSetupDone.setTexture(_buttonSetupDoneTexture);

		using namespace std::placeholders;

		ingameState.tick = std::bind(&MikelepageRuleSet::tick, this);
		ingameState.onMouseMoved = std::bind(&Board::onMouseMoved, &_board, _1);
		ingameState.onMouseButtonPressed = std::bind(&Board::onMouseButtonPressed, &_board, _1);
		ingameState.onMouseButtonReleased = std::bind(&Board::onMouseButtonReleased, &_board, _1);
		ingameState.onKeyPressed = [](const fea::Event::KeyEvent&) { };
		ingameState.onKeyReleased = [](const fea::Event::KeyEvent&) { };

		_board.onTileClicked = std::bind(&MikelepageRuleSet::onTileClicked, this, _1);
		_board.onClickedOutside = std::bind(&MikelepageRuleSet::onClickedOutsideBoard, this, _1);
	}

	void MikelepageRuleSet::tick()
	{
		_board.tick();

		if(_setup)
		{
			for(std::shared_ptr<cyvmath::Piece>& it : _self->_allPieces)
			{
				std::shared_ptr<RenderedPiece> tmp = std::dynamic_pointer_cast<RenderedPiece>(it);
				assert(tmp);
				_renderer.queue(*tmp);
			}

			if(_self->_setupComplete)
				_renderer.queue(_buttonSetupDone);
		}
		else
		{
			for(std::shared_ptr<RenderedPiece> it : _allPieces)
				_renderer.queue(*it);
		}
	}

	void MikelepageRuleSet::onTileClicked(const Tile& tile)
	{
		// temporary workaround [TODO]
		#define cloneCoord(uq_ptr) \
			make_unique<Coordinate>(*uq_ptr)

		resetPossibleTargets();

		// a non-selected tile was clicked
		if(!_selectedTile.first || *tile.first != *_selectedTile.first)
		{
			// there already was a tile selected
			if(_selectedTile.first)
			{
				PlayersColor colorSelf = _self->_color;
				PlayersColor colorOp = (colorSelf == PLAYER_WHITE ? PLAYER_BLACK : PLAYER_WHITE);

				PieceMap::iterator itStart = _self->_activePieces.find(cloneCoord(_selectedTile.first));
				PieceMap::const_iterator itTarget[2] {
						_players[colorSelf]->getActivePieces().find(cloneCoord(tile.first)),
						_players[colorOp]->getActivePieces().find(cloneCoord(tile.first))
					};

				// the selected tile has a piece of the player on it
				if(itStart != _self->_activePieces.end())
				{
					// the clicked tile has no piece on it
					if(itTarget[0] == _players[colorSelf]->getActivePieces().end() &&
					   itTarget[1] == _players[colorOp]->getActivePieces().end())
					{
						// try to move the piece from selected piece to clicked piece
						// this doesn't work without clone(), though I don't know why
						itStart->second->moveTo(cloneCoord(tile.first), !_setup);

						if(_setup)
							_self->checkSetupComplete();

						_selectedTile.second->setColor(_board.getTileColor(_selectedTile.first, _setup));

						tile.second->setColor(_board.getTileColor(tile.first, _setup) + fea::Color(48, 48, 48));

						_selectedTile = Board::noTile();
						return;
					}
					// the clicked piece has an opponents piece on it
					else if(itTarget[1] != _players[colorOp]->getActivePieces().end())
					{
						// TODO: ATTACK!
						_selectedTile.second->setColor(_board.getTileColor(_selectedTile.first, _setup));

						_selectedTile = Board::noTile();
						return;
					}
				}
			}

			// if return wasn't executed, that means either there
			// was no tile selected before or the tile selected
			// before didn't have a piece on it; both means we
			// now move the selection to the clicked tile.

			// reset color of old highlighted tile
			if(_selectedTile.first)
				_selectedTile.second->setColor(_board.getTileColor(_selectedTile.first, _setup));

			tile.second->setColor((_board.getTileColor(tile.first, _setup) + fea::Color(192, 0, 0))
				- fea::Color(0, 64, 64, 0));

			// if the new selected tile has a piece on it and we are no
			// more in the setup, display all possible target tiles
			if(!_setup)
			{
				auto& pieces = _self->getActivePieces();
				auto it = pieces.find(cloneCoord(tile.first));
				if(it != pieces.end()) // the tile clicked on holds an own piece
				{
					auto piece = std::dynamic_pointer_cast<cyvmath::mikelepage::Piece>(it->second);
					assert(piece);

					for(auto targetC : piece->getPossibleTargetTiles())
					{
						fea::Quad* targetQ = _board.getTileAt(targetC);
						targetQ->setColor((_board.getTileColor(targetC, false)
							+ fea::Color(0, 0, 192)) - fea::Color(64, 64, 0, 0));

						_possibleTargets.emplace(make_unique<Coordinate>(targetC), targetQ);
					}
				}
			}

			_selectedTile = std::make_pair(cloneCoord(tile.first), tile.second);
		}
		else // selected tile was clicked again - unselect it
		{
			// giving it the hovered color may be weird on
			// touch screens, this should be fixed somewhen
			tile.second->setColor(_board.getTileColor(tile.first, _setup) + fea::Color(48, 48, 48, 0));

			_selectedTile = Board::noTile();
		}
	}

	void MikelepageRuleSet::onClickedOutsideBoard(const fea::Event::MouseButtonEvent& event)
	{
		resetPossibleTargets();

		// a tile is selected
		if(_selectedTile.first)
		{
			_selectedTile.second->setColor(_board.getTileColor(_selectedTile.first, _setup));
			_selectedTile = Board::noTile();
		}
		// 'Setup done' button clicked (while being visible)
		else if(_self->setupComplete() && mouseOver(_buttonSetupDone, {event.x, event.y}))
			exitSetup();
	}

	void MikelepageRuleSet::placePiecesSetup()
	{
		#define coord(x, y) \
			Coordinate::create((x), (y))

		typedef std::pair<PieceType, std::shared_ptr<Coordinate>> Position;

		static std::map<PlayersColor, std::vector<Position>> defaultPiecePositions {
			{
				PLAYER_WHITE, {
					{PIECE_MOUNTAIN,    coord(0, 10)},
					{PIECE_MOUNTAIN,    coord(1, 10)},
					{PIECE_MOUNTAIN,    coord(2, 10)},
					{PIECE_MOUNTAIN,    coord(3, 10)},
					{PIECE_MOUNTAIN,    coord(4, 10)},
					{PIECE_MOUNTAIN,    coord(5, 10)},

					{PIECE_TREBUCHET,   coord(1, 8)},
					{PIECE_TREBUCHET,   coord(2, 8)},
					{PIECE_ELEPHANT,    coord(3, 8)},
					{PIECE_ELEPHANT,    coord(4, 8)},
					{PIECE_HEAVY_HORSE, coord(5, 8)},
					{PIECE_HEAVY_HORSE, coord(6, 8)},

					{PIECE_RABBLE,      coord(1, 7)},
					{PIECE_RABBLE,      coord(2, 7)},
					{PIECE_RABBLE,      coord(3, 7)},
					{PIECE_KING,        coord(4, 7)},
					{PIECE_RABBLE,      coord(5, 7)},
					{PIECE_RABBLE,      coord(6, 7)},
					{PIECE_RABBLE,      coord(7, 7)},

					{PIECE_CROSSBOWS,   coord(2, 6)},
					{PIECE_CROSSBOWS,   coord(3, 6)},
					{PIECE_SPEARS,      coord(4, 6)},
					{PIECE_SPEARS,      coord(5, 6)},
					{PIECE_LIGHT_HORSE, coord(6, 6)},
					{PIECE_LIGHT_HORSE, coord(7, 6)},

					// dragon starts outside the board
					{PIECE_DRAGON,      nullptr}
				}
			},
			{
				PLAYER_BLACK, {
					{PIECE_MOUNTAIN,    coord(10, 0)},
					{PIECE_MOUNTAIN,    coord(9,  0)},
					{PIECE_MOUNTAIN,    coord(8,  0)},
					{PIECE_MOUNTAIN,    coord(7,  0)},
					{PIECE_MOUNTAIN,    coord(6,  0)},
					{PIECE_MOUNTAIN,    coord(5,  0)},

					{PIECE_TREBUCHET,   coord(9, 2)},
					{PIECE_TREBUCHET,   coord(8, 2)},
					{PIECE_ELEPHANT,    coord(7, 2)},
					{PIECE_ELEPHANT,    coord(6, 2)},
					{PIECE_HEAVY_HORSE, coord(5, 2)},
					{PIECE_HEAVY_HORSE, coord(4, 2)},

					{PIECE_RABBLE,      coord(9, 3)},
					{PIECE_RABBLE,      coord(8, 3)},
					{PIECE_RABBLE,      coord(7, 3)},
					{PIECE_KING,        coord(6, 3)},
					{PIECE_RABBLE,      coord(5, 3)},
					{PIECE_RABBLE,      coord(4, 3)},
					{PIECE_RABBLE,      coord(3, 3)},

					{PIECE_CROSSBOWS,   coord(8, 4)},
					{PIECE_CROSSBOWS,   coord(7, 4)},
					{PIECE_SPEARS,      coord(6, 4)},
					{PIECE_SPEARS,      coord(5, 4)},
					{PIECE_LIGHT_HORSE, coord(4, 4)},
					{PIECE_LIGHT_HORSE, coord(3, 4)},

					// dragon starts outside the board
					{PIECE_DRAGON,      nullptr}
				}
			}};

		#undef coord

		PlayersColor color = _self->_color;

		for(auto& it : defaultPiecePositions[color])
		{
			dc::unique_ptr<Coordinate> tmpCoord;
			if(it.second)
				tmpCoord = dc::make_unique<Coordinate>(*it.second);

			std::shared_ptr<RenderedPiece> tmpPiece(new RenderedPiece
				(it.first, tmpCoord.clone(), color, _self->_activePieces, _board));

			if(it.second) // not null - one of the first 25 pieces
			{
				_self->_activePieces.emplace(std::move(tmpCoord), tmpPiece);
			}
			else // dragon, 26th piece
			{
				glm::uvec2 boardSize = _board.getSize();
				glm::uvec2 boardPos = _board.getPosition();

				glm::uvec2 dragonPos = {boardPos.x, boardPos.y + boardSize.y - _board.getTileSize().y};
				dragonPos += glm::vec2(8, 4); // TODO

				tmpPiece->setPosition(dragonPos);

				_self->_inactivePieces.push_back(tmpPiece);
			}
			_allPieces.push_back(tmpPiece);
			_self->_allPieces.push_back(tmpPiece);
		}
	}

	void MikelepageRuleSet::exitSetup()
	{
		_setup = false;

		if(_self->_color == PLAYER_WHITE)
		{
			_board.updateTileColors(5, 11);
			//placePiecesSetup(PLAYER_BLACK);
		}
		else
		{
			_board.updateTileColors(0, 5);
			//placePiecesSetup(PLAYER_WHITE);
		}
	}

	void MikelepageRuleSet::resetPossibleTargets()
	{
		for(auto& it : _possibleTargets)
			it.second->setColor(_board.getTileColor(it.first, _setup));

		_possibleTargets.clear();
	}
}
