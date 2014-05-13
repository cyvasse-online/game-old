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

// lodepng helper function
#include "texturemaker.hpp"

MikelepageRuleSet::Piece::Piece(MikelepageRuleSet::PieceType type, MikelepageRuleSet::PlayersColor c)
	: _type(type)
	, _quad({48.0f, 40.0f})
{
	static std::string colorStr[2] = {"white", "black"};

	std::string texturePath = "icons/" + colorStr[c] + "/";

	switch(type)
	{
		case PIECE_MOUNTAIN:    texturePath += "mountain.png";    break;
		case PIECE_RABBLE:      texturePath += "rabble.png";      break;
		case PIECE_CROSSBOWS:   texturePath += "crossbows.png";   break;
		case PIECE_SPEARS:      texturePath += "spears.png";      break;
		case PIECE_LIGHT_HORSE: texturePath += "light_horse.png"; break;
		case PIECE_TREBUCHET:   texturePath += "trebuchet.png";   break;
		case PIECE_ELEPHANT:    texturePath += "elephant.png";    break;
		case PIECE_HEAVY_HORSE: texturePath += "heavy_horse.png"; break;
		case PIECE_DRAGON:      texturePath += "dragon.png";      break;
		case PIECE_KING:        texturePath += "king.png";        break;
	}

	_texture = makeTexture(texturePath, 48, 40);
	_quad.setTexture(_texture);
}

MikelepageRuleSet::MikelepageRuleSet(fea::Renderer2D& renderer)
	: RuleSet(renderer, new HexagonalBoard<6>(renderer, {40, 40}, {800 - 2 * 40, 600 - 2 * 40}))
	, _setup(true)
	, _dragonAlive{true, true}
{
	// should better be defined once for HexagonalBoard
	// and this, but defining it twice is okay for now
	// + 6 because the icons are only 48px wide, while
	// the tiles are 60px wide
	// + 40 because piece graphics should start in the
	// second row of tiles
	int xOffset = 40 + 6, yOffset = 40 + 40;

	// creating one RuleSet means starting a new game,
	// so there are no setup() and destroy() functions
	// and the setup is done in this constructor.
	for(int player = 0; player < 2; player++)
	{
		for(int i = 0; i < 3; i++)
		{
			Piece* tmpRabble = new Piece(PIECE_RABBLE, (PlayersColor) player);
			tmpRabble->getQuad().setPosition({xOffset + 120 + i * 60, yOffset + 120});
			_inactivePieces[player].push_back(tmpRabble);
		}

		Piece* tmpKing = new Piece(PIECE_KING, (PlayersColor) player);
		tmpKing->getQuad().setPosition({xOffset + 300, yOffset + 120});
		_inactivePieces[player].push_back(tmpKing);

		for(int i = 0; i < 3; i++)
		{
			Piece* tmpRabble = new Piece(PIECE_RABBLE, (PlayersColor) player);
			tmpRabble->getQuad().setPosition({xOffset + 360 + i * 60, yOffset + 120});
			_inactivePieces[player].push_back(tmpRabble);
		}
		for(int i = 0; i < 2; i++)
		{
			Piece* tmpCrossbows = new Piece(PIECE_CROSSBOWS, (PlayersColor) player);
			tmpCrossbows->getQuad().setPosition({xOffset + 150 + i * 60, yOffset + 80});
			_inactivePieces[player].push_back(tmpCrossbows);
		}
		for(int i = 0; i < 2; i++)
		{
			Piece* tmpSpears = new Piece(PIECE_SPEARS, (PlayersColor) player);
			tmpSpears->getQuad().setPosition({xOffset + 270 + i * 60, yOffset + 80});
			_inactivePieces[player].push_back(tmpSpears);
		}
		for(int i = 0; i < 2; i++)
		{
			Piece* tmpLightHorse = new Piece(PIECE_LIGHT_HORSE, (PlayersColor) player);
			tmpLightHorse->getQuad().setPosition({xOffset + 390 + i * 60, yOffset + 80});
			_inactivePieces[player].push_back(tmpLightHorse);
		}
		for(int i = 0; i < 2; i++)
		{
			Piece* tmpTrebuchet = new Piece(PIECE_TREBUCHET, (PlayersColor) player);
			tmpTrebuchet->getQuad().setPosition({xOffset + 150 + i * 60, yOffset + 40});
			_inactivePieces[player].push_back(tmpTrebuchet);
		}
		for(int i = 0; i < 2; i++)
		{
			Piece* tmpElephant = new Piece(PIECE_ELEPHANT, (PlayersColor) player);
			tmpElephant->getQuad().setPosition({xOffset + 270 + i * 60, yOffset + 40});
			_inactivePieces[player].push_back(tmpElephant);
		}
		for(int i = 0; i < 2; i++)
		{
			Piece* tmpHeavyHorse = new Piece(PIECE_HEAVY_HORSE, (PlayersColor) player);
			tmpHeavyHorse->getQuad().setPosition({xOffset + 390 + i * 60, yOffset + 40});
			_inactivePieces[player].push_back(tmpHeavyHorse);
		}
		for(int i = 0; i < 6; i++)
		{
			Piece* tmpMountain = new Piece(PIECE_MOUNTAIN, (PlayersColor) player);
			tmpMountain->getQuad().setPosition({xOffset + 150 + i * 60, yOffset});
			_inactivePieces[player].push_back(tmpMountain);
		}
	}
}

void MikelepageRuleSet::tick()
{
	_board->tick();

	pieceVec* piecesToBeRendered;
	if(_setup)
		piecesToBeRendered = &_inactivePieces[0];
	else
		piecesToBeRendered = &_allActivePieces;

	for(Piece* it : *piecesToBeRendered)
	{
		_renderer.queue(*it);
	}
}
