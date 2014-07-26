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

#ifdef EMSCRIPTEN
	#include <emscripten.h>
#endif
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
		, _hoveringOwnDragonTile(false)
		, _hoveringOpDragonTile(false)
		, _selectedPiece(nullptr)
	{
		_players.emplace(color, std::make_shared<LocalPlayer>(color, _activePieces));
		_players.emplace(!color, std::make_shared<RemotePlayer>(!color, *this));
		_self = std::dynamic_pointer_cast<LocalPlayer>(_players[color]);
		_op   = _players[!color];
		assert(_self);

		glm::uvec2 boardSize = _board.getSize();
		glm::uvec2 boardPos = _board.getPosition();

		auto tmpTexture = makeTexture("setup-done.png");

		_buttonSetupDoneTexture = std::move(tmpTexture.first);
		_buttonSetupDone.setPosition(boardPos + boardSize - tmpTexture.second);
		_buttonSetupDone.setSize(tmpTexture.second); // hardcoded for now, can be done properly somewhen else
		_buttonSetupDone.setTexture(_buttonSetupDoneTexture);

		_ownDragonTile.setSize(_board.getTileSize());
		_ownDragonTile.setColor(Board::tileColors[1]);
		_ownDragonTile.setPosition(
			{boardPos.x, boardPos.y + boardSize.y - _board.getTileSize().y}
		);

		_opDragonTile.setSize(_board.getTileSize());
		_opDragonTile.setColor(Board::tileColorsDark[1]);
		_opDragonTile.setPosition(
			{boardPos.x + boardSize.x - _board.getTileSize().x, boardPos.y}
		);

		placePiecesSetup();

		using namespace std::placeholders;

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

	void RenderedMatch::setStatus(const std::string& text)
	{
		#ifdef EMSCRIPTEN
		EM_ASM_({
			Module.setStatus(Module.Pointer_stringify($0));
		}, text.c_str());
		#endif
	}

	void RenderedMatch::tick()
	{
		_board.tick();

		if(_self->dragonAliveInactive())
			_renderer.queue(_ownDragonTile);
		if(!_setup && _op->dragonAliveInactive())
			_renderer.queue(_opDragonTile);

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

		if(!_setup && _activePlayer != _self->getColor())
			return;

		if(!_selectedPiece)
		{
			// search for a piece on the clicked tile
			auto it = _activePieces.find(coord);

			if(it != _activePieces.end() && it->second->getColor() == _self->getColor())
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

			if(!piece) // there is no piece on the clicked tile
			{
				tryMovePiece(_selectedPiece, coord);
				_selectedPiece.reset();
			}
			// there is a piece of the player on the clicked tile
			else if(piece->getColor() == _self->getColor())
			{
				// move the focus to the clicked tile
				resetSelectedTile();
				clearPossibleTargetTiles();

				_selectedPiece = piece;

				_board.highlightTile(coord, "red", _setup);
				if(!_setup)
					showPossibleTargetTiles();
			}
			// there is an opponents piece on the clicked tile
			else
			{
				// TODO: attacking code
			}
		}
	}

	void RenderedMatch::onMouseMoveOutsideBoard(const fea::Event::MouseMoveEvent& event)
	{
		if(_self->dragonAliveInactive() &&
		   mouseOver(_ownDragonTile, {event.x, event.y}))
		{
			if(!_hoveringOwnDragonTile)
			{
				_ownDragonTile.setColor(_ownDragonTile.getColor() + Board::hoverColor);
				_hoveringOwnDragonTile = true;
			}
		}
		else if(_hoveringOwnDragonTile)
		{
			_ownDragonTile.setColor(_ownDragonTile.getColor() - Board::hoverColor);
			_hoveringOwnDragonTile = false;
		}
		else if(_op->dragonAliveInactive() &&
		        mouseOver(_opDragonTile, {event.x, event.y}))
		{
			if(!_hoveringOpDragonTile)
			{
				_opDragonTile.setColor(_opDragonTile.getColor() + Board::hoverColor);
				_hoveringOpDragonTile = true;
			}
		}
		else if(_hoveringOpDragonTile)
		{
			_opDragonTile.setColor(_opDragonTile.getColor() - Board::hoverColor);
			_hoveringOpDragonTile = false;
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
			// send before modifying _activePieces, so it can be sent completely
			_self->sendLeaveSetup();
			tryLeaveSetup();
		}
		else if(_self->dragonAliveInactive() &&
		        mouseOver(_ownDragonTile, {event.x, event.y}) &&
		        (_setup || _activePlayer == _self->getColor()))
		{
			for(auto it : _self->getInactivePieces())
			{
				if(it->getType() == PieceType::DRAGON)
					_selectedPiece = it;
			}
			Board::highlight(_ownDragonTile, Board::tileColors[1], "red");

			if(!_setup)
				showPossibleTargetTiles();
		}
	}

	void RenderedMatch::placePiecesSetup()
	{
		#define coord(x, y) \
			Coordinate::create((x), (y))

		typedef std::pair<PieceType, std::shared_ptr<Coordinate>> Position;

		static std::map<PlayersColor, std::vector<Position>> defaultPiecePositions {
			{
				PlayersColor::WHITE, {
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
				}
			},
			{
				PlayersColor::BLACK, {
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
				assert(_self->getInactivePieces().empty());

				tmpPiece->getQuad().setPosition(_ownDragonTile.getPosition());
				_self->getInactivePieces().push_back(tmpPiece);
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

		PieceVec& opInactivePieces = _op->getInactivePieces();
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
				assert(it->getType() == PieceType::DRAGON);
				assert(!unpositionedDragon); // there can only be one dragon
				unpositionedDragon = it;

				std::shared_ptr<RenderedPiece> tmp = std::dynamic_pointer_cast<RenderedPiece>(it);
				assert(tmp);
				tmp->getQuad().setPosition(_opDragonTile.getPosition());
				_piecesToRender.push_back(tmp);
			}
		}

		// this is probably faster than erasing one single element in each
		// iteration of the above loop except in the unpositioned dragon case
		if(unpositionedDragon)
			opInactivePieces.assign({unpositionedDragon});
		else
		{
			opInactivePieces.clear();
			_op->dragonBroughtOut();
		}

		// TODO: Rewrite when Terrain is added
		int nFortresses = 0;
		for(auto it : _piecesToRender)
		{
			assert(it);
			if(it->getType() == PieceType::KING)
			{
				auto coord = it->getCoord();
				assert(coord);

				_fortressPositions.emplace(it->getColor(), *coord);
				nFortresses++;
			}
		}
		assert(nFortresses == 2);

		if(_self->_color == PlayersColor::WHITE)
			_board.resetTileColors(5, 11);
		else
			_board.resetTileColors(0, 5);

		_opDragonTile.setColor(Board::tileColors[1]);

		updateTurnStatus();
	}

	void RenderedMatch::tryMovePiece(std::shared_ptr<Piece> piece, Coordinate coord)
	{
		assert(piece);

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

			if(!_setup)
			{
				_activePlayer = !_activePlayer;
				updateTurnStatus();
			}
		}
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

			if(_selectedPiece->getColor() == _self->getColor())
				_ownDragonTile.setColor(Board::tileColors[1]);
			else
				_opDragonTile.setColor(_setup ? Board::tileColorsDark[1] : Board::tileColors[1]);
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
}
