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

MikelepageRuleSet::RenderedPiece::RenderedPiece(PieceType type, PlayersColor c, Board& board)
	: Piece(type)
	, _board(board)
	, _map(nullptr)
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

	_texture = makeTexture(("icons/" + colorStr[c] + "/" + fileNames.at(type)), 48, 40);
	_quad.setTexture(_texture);
}

void MikelepageRuleSet::RenderedPiece::moveTo(Coordinate c, bool setup)
{
	if(!setup)
	{
		// Check if the movement is legal
		// (Use assert for the check or return if the check fails)
	}

	_cPos = std::unique_ptr<Coordinate>(new Coordinate(c));

	glm::vec2 position = _board.getTilePosition(*_cPos);
	position += 8; // TODO

	_quad.setPosition(position);
}

MikelepageRuleSet::MikelepageRuleSet(fea::Renderer2D& renderer, PlayersColor playersColor)
	// TODO: parametrize the numerical stuff
	: RuleSet(renderer)
	, _board(renderer, {800 - 2 * 40, 600 - 2 * 40}, {40, 40})
	, _playersColor(playersColor)
	, _setup(true)
	, _dragonAlive{true, true}
{
	const glm::vec2& tileSize = _board.getTileSize();

	// creating one RuleSet means starting a new game,
	// so there are no setup() and destroy() functions
	// and the setup is done in this constructor.

	// {type, hex-coordinate, mid-coordinate}
	// if the last bool is true, the piece will not be directly on one
	// tile, but have its horizontal center on the line between the
	// tile with the given coordinate and the next tile to the right
	static std::vector<std::tuple<PieceType, std::pair<int8_t, int8_t>, bool>> defaultPiecePositions = {
			{PIECE_MOUNTAIN, std::make_pair(0,9), true},
			{PIECE_MOUNTAIN, std::make_pair(1,9), true},
			{PIECE_MOUNTAIN, std::make_pair(2,9), true},
			{PIECE_MOUNTAIN, std::make_pair(3,9), true},
			{PIECE_MOUNTAIN, std::make_pair(4,9), true},
			{PIECE_MOUNTAIN, std::make_pair(5,9), true},

			{PIECE_TREBUCHET,   std::make_pair(1,8), false},
			{PIECE_TREBUCHET,   std::make_pair(2,8), false},
			{PIECE_ELEPHANT,    std::make_pair(3,8), false},
			{PIECE_ELEPHANT,    std::make_pair(4,8), false},
			{PIECE_HEAVY_HORSE, std::make_pair(5,8), false},
			{PIECE_HEAVY_HORSE, std::make_pair(6,8), false},

			{PIECE_CROSSBOWS,   std::make_pair(1,7), true},
			{PIECE_CROSSBOWS,   std::make_pair(2,7), true},
			{PIECE_SPEARS,      std::make_pair(3,7), true},
			{PIECE_SPEARS,      std::make_pair(4,7), true},
			{PIECE_LIGHT_HORSE, std::make_pair(5,7), true},
			{PIECE_LIGHT_HORSE, std::make_pair(6,7), true},

			{PIECE_RABBLE, std::make_pair(1,6), true},
			{PIECE_RABBLE, std::make_pair(2,6), true},
			{PIECE_RABBLE, std::make_pair(3,6), true},
			{PIECE_KING,   std::make_pair(4,6), true},
			{PIECE_RABBLE, std::make_pair(5,6), true},
			{PIECE_RABBLE, std::make_pair(6,6), true},
			{PIECE_RABBLE, std::make_pair(7,6), true}
		};

	for(auto it : defaultPiecePositions)
	{
		RenderedPiece* tmpPiece = new RenderedPiece(std::get<0>(it), _playersColor, _board);

		std::unique_ptr<Coordinate> c = Coordinate::create(std::get<1>(it));
		assert(c); // not null

		glm::vec2 position = _board.getTilePosition(*c);
		glm::vec2 tileSize = _board.getTileSize();
		// TODO: piece graphics should be scaled, after
		// that this constant should also be changed
		position.x += 8;
		if(std::get<2>(it))
			position.x += _board.getTileSize().x / 2;

		tmpPiece->_quad.setPosition(position);

		_inactivePieces[_playersColor].push_back(tmpPiece);
		_allPieces.push_back(tmpPiece);
	}
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
	for(RenderedPiece* it : _inactivePieces[0])
	{
		_renderer.queue(*it);
	}
}

void MikelepageRuleSet::tickPlaying()
{
	for(RenderedPiece* it : _allPieces)
	{
		_renderer.queue(*it);
	}
}

void MikelepageRuleSet::processMouseEvent(fea::Event& event)
{
	assert(event.type == fea::Event::MOUSEBUTTONPRESSED ||
	       event.type == fea::Event::MOUSEBUTTONRELEASED ||
	       event.type == fea::Event::MOUSEMOVED);

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

	std::unique_ptr<Coordinate> c = _board.getCoordinate(std::make_pair(mX, mY));

	if(c)
	{
		static fea::Quad* lastQuad = nullptr;
		static fea::Color lastQuadColor;

		fea::Quad* quad = _board.getTileAt(*c);

		if(lastQuad != quad)
		{
			if(lastQuad)
				lastQuad->setColor(lastQuadColor);

			lastQuad = quad;
			lastQuadColor = quad->getColor();

			quad->setColor({0.0f, 0.5f, 0.0f});
		}
	}
}
