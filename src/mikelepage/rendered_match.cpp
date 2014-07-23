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

#include "common.hpp"
#include "ingame_state.hpp"
#include "local_player.hpp"
#include "rendered_piece.hpp"
#include "remote_player.hpp"
#include "texturemaker.hpp" // lodepng helper function

using namespace cyvmath::mikelepage;

namespace mikelepage
{
	RenderedMatch::RenderedMatch(IngameState& ingameState, fea::Renderer2D& renderer, PlayersColor color)
		: _renderer(renderer)
		, _board(renderer, color)
		, _setupAccepted(false)
		, _selectedPiece(nullptr)
	{
		PlayersColor opColor = (color == PLAYER_WHITE ? PLAYER_BLACK : PLAYER_WHITE);

		_players.emplace(color, std::make_shared<LocalPlayer>(color, _activePieces));
		_players.emplace(opColor, std::make_shared<RemotePlayer>(opColor, *this));
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

		ingameState.tick = std::bind(&RenderedMatch::tick, this);
		ingameState.onMouseMoved = std::bind(&Board::onMouseMoved, &_board, _1);
		ingameState.onMouseButtonPressed = std::bind(&Board::onMouseButtonPressed, &_board, _1);
		ingameState.onMouseButtonReleased = std::bind(&Board::onMouseButtonReleased, &_board, _1);
		ingameState.onKeyPressed = [](const fea::Event::KeyEvent&) { };
		ingameState.onKeyReleased = [](const fea::Event::KeyEvent&) { };

		_board.onTileClicked = std::bind(&RenderedMatch::onTileClicked, this, _1);
		_board.onClickedOutside = std::bind(&RenderedMatch::onClickedOutsideBoard, this, _1);
	}

	void RenderedMatch::tick()
	{
		_board.tick();

		for(std::shared_ptr<RenderedPiece> it : _piecesToRender)
			_renderer.queue(it->getQuad());

		if(_setup && _self->setupComplete() && !_setupAccepted)
			_renderer.queue(_buttonSetupDone);
	}

	void RenderedMatch::onTileClicked(Coordinate coord)
	{
		// if the player left the setup but the opponent isn't ready yet
		if(_setup == _setupAccepted)
			return;

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
				tryMovePiece(_selectedPiece, coord);
				_selectedPiece.reset();
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

	void RenderedMatch::onClickedOutsideBoard(const fea::Event::MouseButtonEvent& event)
	{
		// a piece is selected
		if(_selectedPiece)
		{
			_board.resetTileColor(*_selectedPiece->getCoord(), _setup);
			clearPossibleTargetTiles();

			_selectedPiece.reset();
		}
		// 'Setup done' button clicked (while being visible)
		else if(_self->setupComplete() && !_setupAccepted && mouseOver(_buttonSetupDone, {event.x, event.y}))
		{
			_setupAccepted = true;
			// send before modifying _activePieces, so it can be sent completely
			_self->sendLeaveSetup();
			tryLeaveSetup();
		}
	}

	void RenderedMatch::placePiecesSetup()
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
			std::shared_ptr<RenderedPiece> tmpPiece(new RenderedPiece(it.first, make_unique(it.second), color, *this));

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

				tmpPiece->getQuad().setPosition(dragonPos);

				_self->_inactivePieces.push_back(tmpPiece);
			}
			_piecesToRender.push_back(tmpPiece);
		}
	}

	void RenderedMatch::tryLeaveSetup()
	{
		if(!_setupAccepted) return;

		for(auto it : _players)
			if(!it.second->setupComplete())
				return;

		_setup = false;

		// temporary variable to hold the dragon piece if it isn't
		// "brought out" to the board (else this variable will remain unchanged)
		std::shared_ptr<Piece> unpositionedDragon;

		PlayersColor opColor = _self->getColor() == PLAYER_WHITE ? PLAYER_BLACK : PLAYER_WHITE;
		PieceVec& opInactivePieces = _players[opColor]->getInactivePieces();
		for(auto& it : opInactivePieces)
		{
			if(it->getCoord())
			{
				_activePieces.emplace(*it->getCoord(), it);

				std::shared_ptr<RenderedPiece> tmp = std::dynamic_pointer_cast<RenderedPiece>(it);
				assert(tmp);
				_piecesToRender.push_back(tmp);
			}
			else
			{
				// the piece has to be a dragon, because other pieces
				// have to have a position on the board after setup
				assert(it->getType() == PIECE_DRAGON);
				assert(!unpositionedDragon); // there can only be one dragon
				unpositionedDragon = it;
			}
		}

		// this is probably faster than erasing one single element in each
		// iteration of the above loop (except in the unpositioned dragon case)
		if(unpositionedDragon)
			opInactivePieces.assign({unpositionedDragon});
		else
			opInactivePieces.clear();

		// TODO: Rewrite when Terrain is added
		int nFortresses = 0;
		for(auto it : _piecesToRender)
		{
			assert(it);
			if(it->getType() == PIECE_KING)
			{
				auto coord = it->getCoord();
				assert(coord);

				_fortressPositions.emplace(it->getColor(), *coord);
				nFortresses++;
			}
		}
		assert(nFortresses == 2);

		if(_self->_color == PLAYER_WHITE)
			_board.resetTileColors(5, 11);
		else
			_board.resetTileColors(0, 5);
	}

	void RenderedMatch::tryMovePiece(std::shared_ptr<Piece> piece, Coordinate coord)
	{
		auto oldCoord = piece->getCoord();

		if(piece->moveTo(coord, !_setup))
		{
			// move is valid and was done

			if(piece->getColor() == _self->getColor())
			{
				if(!_setup)
					_self->sendMovePiece(piece, make_unique(oldCoord));

				if(!_setupAccepted)
					_self->checkSetupComplete();
			}

			clearPossibleTargetTiles();
			if(oldCoord)
				_board.resetTileColor(*oldCoord, _setup);
		}
	}

	void RenderedMatch::showPossibleTargetTiles(Coordinate coord)
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

	void RenderedMatch::clearPossibleTargetTiles()
	{
		for(auto it : _possibleTargetTiles)
			_board.resetTileColor(it, _setup);

		_possibleTargetTiles.clear();
	}
}
