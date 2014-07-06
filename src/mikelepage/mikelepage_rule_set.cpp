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
		, _selectedPiece(nullptr)
	{
		PlayersColor opColor = (color == PLAYER_WHITE ? PLAYER_BLACK : PLAYER_WHITE);

		_players.emplace(color, std::make_shared<LocalPlayer>(color, _activePieces));
		_players.emplace(opColor, std::make_shared<RemotePlayer>(opColor, _activePieces));
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
			for(auto it : _self->_allPieces)
			{
				auto tmp = std::dynamic_pointer_cast<RenderedPiece>(it);
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

	void MikelepageRuleSet::onTileClicked(Coordinate coord)
	{
		if(!_selectedPiece)
		{
			// search for a piece on the clicked tile
			auto it = _activePieces.find(coord);

			if(it != _activePieces.end() && it->second->getColor() == _self->getColor())
			{
				// a piece of the player was clicked
				_selectedPiece = it->second;

				_board.highlightTileRed(coord, _setup);
				showPossibleTargetTiles(coord);
			}
		}
		else // a piece is selected
		{
			// determine which piece is on the clicked tile
			std::shared_ptr<Piece> piece;

			auto pieceIt = _activePieces.find(coord);
			if(pieceIt != _activePieces.end())
				piece = pieceIt->second;

			if(!piece) // there is no piece on the clicked tile
			{
				auto oldCoord = _selectedPiece->getCoord();

				// try to move to the clicked tile
				if(_selectedPiece->moveTo(coord, !_setup))
				{
					// move is valid and was done
					_selectedPiece.reset();

					if(_setup)
						_self->checkSetupComplete();

					clearPossibleTargetTiles();
					//if(oldCoord)
						_board.resetTileColor(*oldCoord, _setup);
				}
			}
			// there is a piece of the player on the clicked tile
			else if(piece->getColor() == _self->getColor())
			{
				// move the focus to the clicked tile
				_board.resetTileColor(*_selectedPiece->getCoord(), _setup);
				clearPossibleTargetTiles();

				_selectedPiece = piece;

				_board.highlightTileRed(coord, _setup);
				showPossibleTargetTiles(coord);
			}
			// there is an opponents piece on the clicked tile
			else
			{
				// TODO: attacking code
			}
		}
	}

	void MikelepageRuleSet::onClickedOutsideBoard(const fea::Event::MouseButtonEvent& event)
	{
		// a piece is selected
		if(_selectedPiece)
		{
			_board.resetTileColor(*_selectedPiece->getCoord(), _setup);
			clearPossibleTargetTiles();

			_selectedPiece.reset();
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
			std::shared_ptr<RenderedPiece> tmpPiece(new RenderedPiece(it.first, make_unique(it.second),
			                                        color, _activePieces, _board));

			if(it.second) // not null - one of the first 25 pieces
			{
				_activePieces.emplace(*it.second, tmpPiece);
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
			_board.resetTileColors(5, 11);
			//placePiecesSetup(PLAYER_BLACK);
		}
		else
		{
			_board.resetTileColors(0, 5);
			//placePiecesSetup(PLAYER_WHITE);
		}
	}

	void MikelepageRuleSet::showPossibleTargetTiles(Coordinate coord)
	{
		// only show possible target tiles when not in setup
		if(!_setup)
		{
			auto& pieces = _activePieces;
			auto it = pieces.find(coord);
			if(it != pieces.end() && it->second->getColor() == _self->getColor())
			{
				// the tile clicked on holds a piece of the player
				for(auto targetTile : it->second->getPossibleTargetTiles())
				{
					_board.highlightTileBlue(targetTile, false);

					_possibleTargetTiles.emplace(targetTile);
				}
			}
		}
	}

	void MikelepageRuleSet::clearPossibleTargetTiles()
	{
		for(auto it : _possibleTargetTiles)
			_board.resetTileColor(it, _setup);

		_possibleTargetTiles.clear();
	}
}
