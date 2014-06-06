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
// lodepng helper function
#include "texturemaker.hpp"

using namespace mikelepage;

MikelepageRuleSet::RenderedPiece::RenderedPiece(PieceType type, Coordinate* coord,
                                                PlayersColor color, PieceMap& map, Board& board)
	: Piece(color, type, coord, map)
	, fea::Quad({48.0f, 40.0f})
	, _board(board)
{
	static std::string colorStr[2] = {"white", "black"};
	static std::map<PieceType, std::string> fileNames = {
			{PIECE_MOUNTAIN,    "mountain.png"},
			{PIECE_RABBLE,      "rabble.png"},
			{PIECE_CROSSBOWS,   "crossbows.png"},
			{PIECE_SPEARS,      "spears.png"},
			{PIECE_LIGHT_HORSE, "light_horse.png"},
			{PIECE_TREBUCHET,   "trebuchet.png"},
			{PIECE_ELEPHANT,    "elephant.png"},
			{PIECE_HEAVY_HORSE, "heavy_horse.png"},
			{PIECE_DRAGON,      "dragon.png"},
			{PIECE_KING,        "king.png"}
		};

	mTexture = new fea::Texture(makeTexture(("icons/" + colorStr[color] + "/" + fileNames.at(type)), 48, 40));

	glm::vec2 position = _board.getTilePosition(*_coord);
	// TODO: piece graphics should be scaled, after
	// that this constant should also be changed
	position += glm::vec2(8, 4);

	setPosition(position);
}

void MikelepageRuleSet::RenderedPiece::moveTo(Coordinate coord, bool setup)
{
	if(!setup)
	{
		// Check if the movement is legal
		// (Use assert for the check or return if the check fails)
		// TODO
	}

	PieceMap::iterator it = _map.find(*_coord);
	assert(it != _map.end());

	_coord = std::unique_ptr<Coordinate>(new Coordinate(coord));

	_map.erase(it);
	std::pair<PieceMap::iterator, bool> res = _map.emplace(*_coord, this);
	// assert there is no other piece already on coord.
	// the check for that is probably better to do outside this
	// functions, but this line still may change in the future.
	assert(res.second);

	glm::vec2 position = _board.getTilePosition(*_coord);
	position += glm::vec2(8, 4); // TODO

	setPosition(position);
}

MikelepageRuleSet::MikelepageRuleSet(fea::Renderer2D& renderer, PlayersColor playersColor)
	: RuleSet(renderer)
	, Match(playersColor)
	, _board(renderer, playersColor)
{
	placePiecesSetup(_playersColor);

	glm::uvec2 boardSize = _board.getSize();
	glm::uvec2 boardPos = _board.getPosition();
	// hardcoded for now, can be done properly somewhen else
	glm::uvec2 buttonSize = {80, 50};

	_buttonSetupDoneTexture = makeTexture("setup-done.png", buttonSize.x, buttonSize.y);
	_buttonSetupDone.setPosition(boardPos + boardSize - buttonSize);
	_buttonSetupDone.setSize(buttonSize);
	_buttonSetupDone.setTexture(_buttonSetupDoneTexture);
}

MikelepageRuleSet::~MikelepageRuleSet()
{
	for(RenderedPiece* it : _allPieces[PLAYER_WHITE])
		delete it;

	for(RenderedPiece* it : _allPieces[PLAYER_BLACK])
		delete it;
}

void MikelepageRuleSet::tick()
{
	_board.tick();

	if(_setup)
	{
		for(RenderedPiece* it : _allPieces[_playersColor])
			_renderer.queue(*it);

		if(_setupComplete)
			_renderer.queue(_buttonSetupDone);
	}
	else
	{
		for(RenderedPiece* it : _allPieces[PLAYER_WHITE])
			_renderer.queue(*it);

		for(RenderedPiece* it : _allPieces[PLAYER_BLACK])
			_renderer.queue(*it);
	}
}

void MikelepageRuleSet::processEvent(fea::Event& event)
{
	// assert that we are processing a mouse event (for now)
	int mX, mY; // mouse x and y coordinates
	if(event.type == fea::Event::MOUSEMOVED)
	{
		mX = event.mouseMove.x;
		mY = event.mouseMove.y;
	}
	else
	{
		mX = event.mouseButton.x;
		mY = event.mouseButton.y;
	}

	static std::pair<std::unique_ptr<Coordinate>, fea::Quad*> lastTile
		= std::make_pair(std::unique_ptr<Coordinate>(), nullptr);
	static std::pair<std::unique_ptr<Coordinate>, fea::Quad*> selectedTile
		= std::make_pair(std::unique_ptr<Coordinate>(), nullptr);

	std::unique_ptr<Coordinate> pCoord = _board.getCoordinate({mX, mY});
	fea::Quad* quad = _board.getTileAt(*pCoord);

	switch(event.type)
	{
		case fea::Event::MOUSEMOVED:
			if(pCoord) // mouse hovered on tile (*pCoord)
			{
				if(*pCoord != *lastTile.first)
				{
					// reset color of old highlighted tile
					if(lastTile.first && *lastTile.first != *selectedTile.first)
						lastTile.second->setColor(_board.getTileColor(*lastTile.first, _setup));

					if(*pCoord != *selectedTile.first)
					{
						quad->setColor((_board.getTileColor(*pCoord, _setup) + fea::Color(0.0f, 0.7f, 0.0f))
							- fea::Color(0.3f, 0.0f, 0.3f, 0.0f));

						// don't access pCoord in this function after it was moved!
						lastTile = std::make_pair(std::move(pCoord), quad);
					}
				}
			}
			else if(lastTile.first && *lastTile.first != *selectedTile.first)
			{
				// mouse is outside the board and one tile is still marked with the hover effect
				lastTile.second->setColor(_board.getTileColor(*lastTile.first, _setup));
				lastTile = std::make_pair(std::unique_ptr<Coordinate>(), nullptr);
			}
			break;
		case fea::Event::MOUSEBUTTONPRESSED:
			// this stuff should be moved to MOUSEBUTTONRELEASED when
			// we check if the mouse is still on the same tile there
			if(pCoord) // clicked on the tile *pCoord
			{
				// a non-selected tile was clicked
				if(!selectedTile.first || *pCoord != *selectedTile.first)
				{
					// there already was a tile selected
					if(selectedTile.first)
					{
						PieceMap::iterator it1 = _activePieces.find(*selectedTile.first);
						PieceMap::iterator it2 = _activePieces.find(*pCoord);

						// the selected tile has a piece of the player on it
						if(it1 != _activePieces.end() && it1->second->getPlayersColor() == _playersColor)
						{
							// the clicked tile has no piece on it
							if(it2 == _activePieces.end())
							{
								// try to move the piece from selected piece to clicked piece
								RenderedPiece* tmpPiece = dynamic_cast<RenderedPiece*>(it1->second);
								assert(tmpPiece);
								tmpPiece->moveTo(*pCoord, _setup);

								if(_setup)
									checkSetupComplete();

								selectedTile.second->setColor(_board.getTileColor(*selectedTile.first, _setup));

								selectedTile = std::make_pair(std::unique_ptr<Coordinate>(), nullptr);
								return;
							}
							// the clicked piece has a piece of the opponent on it
							else if(it2->second->getPlayersColor() != _playersColor)
							{
								// TODO: ATTACK!
								selectedTile.second->setColor(_board.getTileColor(*selectedTile.first, _setup));

								selectedTile = std::make_pair(std::unique_ptr<Coordinate>(), nullptr);
								return;
							}
						}
					}

					// if return wasn't executed, that means either there
					// was no tile selected before or the tile selected
					// before didn't have a piece on it; both means we
					// now move the selection to the clicked tile.

					// reset color of old highlighted tile
					if(selectedTile.first)
						selectedTile.second->setColor(_board.getTileColor(*selectedTile.first, _setup));

					quad->setColor((_board.getTileColor(*pCoord, _setup) + fea::Color(0.7f, 0.0f, 0.0f))
						- fea::Color(0.0f, 0.3f, 0.3f, 0.0f));

					// don't access pCoord in this function after it was moved!
					selectedTile = std::make_pair(std::move(pCoord), quad);
				}
				else // selected tile was clicked again - unselect it
				{
					// giving it the hovered color may be weird on
					// touch screens, this should be fixed somewhen
					quad->setColor((_board.getTileColor(*pCoord, _setup) + fea::Color(0.0f, 0.7f, 0.0f))
							- fea::Color(0.3f, 0.0f, 0.3f, 0.0f));

					// don't access pCoord in this function after it was moved!
					lastTile = std::make_pair(std::move(pCoord), quad);

					selectedTile = std::make_pair(std::unique_ptr<Coordinate>(), nullptr);
				}
			}
			else // clicked outside the board
			{
				// a tile is selected
				if(selectedTile.first)
				{
					selectedTile.second->setColor(_board.getTileColor(*lastTile.first, _setup));
					selectedTile = std::make_pair(std::unique_ptr<Coordinate>(), nullptr);
				}

				// 'Setup done' button pressed (while being visible)
				if(_setupComplete && mouseOver(_buttonSetupDone, event))
					exitSetup();
			}
			break;
	}
}

void MikelepageRuleSet::placePiecesSetup(PlayersColor playersColor)
{
#define coord(x, y) \
	Coordinate::create((x), (y))

	static std::vector<std::pair<PieceType, std::shared_ptr<Coordinate>>> defaultPiecePositions[2] {
		{ // PLAYER_WHITE
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
		},
		{ // PLAYER_BLACK
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
		}};

#undef coord

	// TODO: remove the '!' when there is a better
	// alternative for testing the game without
	// manually doing the setup over and over again
	for(auto& it : defaultPiecePositions[!playersColor])
	{
		Coordinate tmpCoord(*it.second); // create a copy

		RenderedPiece* tmpPiece = new RenderedPiece
			(it.first, new Coordinate(tmpCoord), playersColor, _activePieces, _board);

		if(it.second) // not null - one of the first 25 pieces
		{
			_activePieces.emplace(tmpCoord, tmpPiece);
		}
		else // dragon, 26th piece
		{
			glm::uvec2 boardSize = _board.getSize();
			glm::uvec2 boardPos = _board.getPosition();

			glm::uvec2 dragonPos = {boardPos.x, boardPos.y + boardSize.y - _board.getTileSize().y};
			dragonPos += glm::vec2(8, 4); // TODO

			tmpPiece->setPosition(dragonPos);
		}
		_allPieces[playersColor].push_back(tmpPiece);
	}
}

void MikelepageRuleSet::exitSetup()
{
	_setup = false;

	if(_playersColor == PLAYER_WHITE)
	{
		_board.updateTileColors(5, 11);
		// for test purposes only - does very weird stuff herein, don't try to understand
		placePiecesSetup(PLAYER_BLACK);
	}
	else
	{
		_board.updateTileColors(0, 5);
		// for test purposes only - does very weird stuff herein, don't try to understand
		placePiecesSetup(PLAYER_WHITE);
	}
}
