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

MikelepageRuleSet::RenderedPiece::RenderedPiece(PieceType type, Coordinate* coord, PlayersColor color,
                                                PieceMap& map, Board& board)
	: Piece(type, coord)
	, _map(map)
	, _board(board)
	, _quad({48.0f, 40.0f})
{
	static std::string colorStr[2] = {"white", "black"};
	static std::unordered_map<PieceType, std::string> fileNames = {
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

	_texture = makeTexture(("icons/" + colorStr[color] + "/" + fileNames.at(type)), 48, 40);
	_quad.setTexture(_texture);
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
	position += 8; // TODO

	_quad.setPosition(position);
}

MikelepageRuleSet::MikelepageRuleSet(fea::Renderer2D& renderer, PlayersColor playersColor)
	// TODO: parametrize the numerical stuff
	: RuleSet(renderer)
	, Match(playersColor)
	, _board(renderer, {800 - 2 * 40, 600 - 2 * 40}, {40, 40})
{
	// creating one RuleSet means starting a new game,
	// so there are no setup() and destroy() functions
	// and the setup is done in this constructor.

	// {type, hex-coordinate}
	static std::vector<std::tuple<PieceType, std::pair<int8_t, int8_t>>> defaultPiecePositions = {
			{PIECE_MOUNTAIN, std::make_pair(0,10)},
			{PIECE_MOUNTAIN, std::make_pair(1,10)},
			{PIECE_MOUNTAIN, std::make_pair(2,10)},
			{PIECE_MOUNTAIN, std::make_pair(3,10)},
			{PIECE_MOUNTAIN, std::make_pair(4,10)},
			{PIECE_MOUNTAIN, std::make_pair(5,10)},

			{PIECE_TREBUCHET,   std::make_pair(1,8)},
			{PIECE_TREBUCHET,   std::make_pair(2,8)},
			{PIECE_ELEPHANT,    std::make_pair(3,8)},
			{PIECE_ELEPHANT,    std::make_pair(4,8)},
			{PIECE_HEAVY_HORSE, std::make_pair(5,8)},
			{PIECE_HEAVY_HORSE, std::make_pair(6,8)},

			{PIECE_RABBLE, std::make_pair(1,7)},
			{PIECE_RABBLE, std::make_pair(2,7)},
			{PIECE_RABBLE, std::make_pair(3,7)},
			{PIECE_KING,   std::make_pair(4,7)},
			{PIECE_RABBLE, std::make_pair(5,7)},
			{PIECE_RABBLE, std::make_pair(6,7)},
			{PIECE_RABBLE, std::make_pair(7,7)},

			{PIECE_CROSSBOWS,   std::make_pair(2,6)},
			{PIECE_CROSSBOWS,   std::make_pair(3,6)},
			{PIECE_SPEARS,      std::make_pair(4,6)},
			{PIECE_SPEARS,      std::make_pair(5,6)},
			{PIECE_LIGHT_HORSE, std::make_pair(6,6)},
			{PIECE_LIGHT_HORSE, std::make_pair(7,6)}
		};

	for(auto it : defaultPiecePositions)
	{
		std::unique_ptr<Coordinate> coord = Coordinate::create(std::get<1>(it));
		assert(coord); // not null

		// Create a copy of coord here to avoid troubles
		// with the ownership of the real object
		RenderedPiece* tmpPiece = new RenderedPiece(
				std::get<0>(it), new Coordinate(*coord), _playersColor,
				_activePieces[_playersColor], _board
			);

		glm::vec2 position = _board.getTilePosition(*coord);
		// TODO: piece graphics should be scaled, after
		// that this constant should also be changed
		position.x += 8;

		tmpPiece->_quad.setPosition(position);

		_activePieces[_playersColor].emplace(*coord, tmpPiece);
		_allPieces[_playersColor].push_back(tmpPiece);
	}

	// hardcoded for now, can be done properly somewhen else
	_buttonSetupDone.setPosition({700, 530});
	_buttonSetupDone.setSize({80, 50});
	_buttonSetupDone.setColor({0, 80, 0});
	//_buttonSetupDone.write("Setup done");
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
		tickSetup();
	else
		tickPlaying();
}

void MikelepageRuleSet::tickSetup()
{
	for(RenderedPiece* it : _allPieces[_playersColor])
		_renderer.queue(*it);

	_renderer.queue(_buttonSetupDone);
}

void MikelepageRuleSet::tickPlaying()
{
	for(RenderedPiece* it : _allPieces[PLAYER_WHITE])
		_renderer.queue(*it);

	for(RenderedPiece* it : _allPieces[PLAYER_BLACK])
		_renderer.queue(*it);
}

void MikelepageRuleSet::processEvent(fea::Event& event)
{
	// TODO: This function could use some cleanup
	// and splitting up into smaller functions
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

	std::unique_ptr<Coordinate> c = _board.getCoordinate(std::make_pair(mX, mY));

	if(c)
	{
		fea::Quad* quad = _board.getTileAt(*c);

		switch(event.type)
		{
			case fea::Event::MOUSEMOVED:
				if(*c != *lastTile.first)
				{
					// reset color of old highlighted tile
					if(lastTile.first && *lastTile.first != *selectedTile.first)
						lastTile.second->setColor(_board.getTileColor(*lastTile.first, _setup));

					if(*c != *selectedTile.first)
					{
						quad->setColor((_board.getTileColor(*c, _setup) + fea::Color(0.0f, 0.7f, 0.0f))
							- fea::Color(0.3f, 0.0f, 0.3f, 0.0f));

						// don't access c in this function after it was moved!
						lastTile = std::make_pair(std::move(c), quad);
					}
				}
				break;
			case fea::Event::MOUSEBUTTONPRESSED:
				// this stuff should be moved to MOUSEBUTTONRELEASED when
				// we check if the mouse is still on the same tile there
				if(!selectedTile.first || *c != *selectedTile.first)
				{
					// a non-selected tile was clicked
					if(selectedTile.first)
					{
						PieceMap::iterator it1 = _activePieces[_playersColor].find(*selectedTile.first);
						PieceMap::iterator it2 = _activePieces[_playersColor].find(*c);
						if(it1 != _activePieces[_playersColor].end() && it2 == _activePieces[_playersColor].end())
						{
							// if there is a piece on the previously selected tile,
							// and none on the clicked, the piece is moved (if possible)
							RenderedPiece* tmpPiece = dynamic_cast<RenderedPiece*>(it1->second);
							assert(tmpPiece);
							tmpPiece->moveTo(*c, _setup);

							selectedTile.second->setColor(_board.getTileColor(*selectedTile.first, _setup));

							selectedTile = std::make_pair(std::unique_ptr<Coordinate>(), nullptr);
							return;
						}
					}

					// if return wasn't executed, that means either there
					// was no tile selected before or the tile selected
					// before didn't have a piece on it; both means we
					// now move the selection to the clicked tile.

					// reset color of old highlighted tile
					if(selectedTile.first)
						selectedTile.second->setColor(_board.getTileColor(*selectedTile.first, _setup));

					quad->setColor((_board.getTileColor(*c, _setup) + fea::Color(0.7f, 0.0f, 0.0f))
						- fea::Color(0.0f, 0.3f, 0.3f, 0.0f));

					// don't access c in this function after it was moved!
					selectedTile = std::make_pair(std::move(c), quad);
				}
				else
				{
					// selected tile was clicked again - unselect it
					// giving it the hovered color may be weird on
					// touch screens, this should be fixed somewhen
					quad->setColor((_board.getTileColor(*c, _setup) + fea::Color(0.0f, 0.7f, 0.0f))
							- fea::Color(0.3f, 0.0f, 0.3f, 0.0f));

					// don't access c in this function after it was moved!
					lastTile = std::make_pair(std::move(c), quad);

					selectedTile = std::make_pair(std::unique_ptr<Coordinate>(), nullptr);
				}
		}
	}
	else
	{
		if(lastTile.first)
			lastTile.second->setColor(_board.getTileColor(*lastTile.first, _setup));

		lastTile = std::make_pair(std::unique_ptr<Coordinate>(), nullptr);
	}
}
