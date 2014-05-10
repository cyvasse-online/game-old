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

MikelepageRuleSet::Figure::Figure(MikelepageRuleSet::FigureType type, MikelepageRuleSet::PlayersColor c)
	: _type(type)
	, _quad({48.0f, 40.0f})
{
	static std::string colorStr[2] = {"white", "black"};

	std::string texturePath = "icons/" + colorStr[c] + "/";

	switch(type)
	{
		case FIGURE_MOUNTAIN:    texturePath += "mountain.png";    break;
		case FIGURE_RABBLE:      texturePath += "rabble.png";      break;
		case FIGURE_CROSSBOWS:   texturePath += "crossbows.png";   break;
		case FIGURE_SPEARS:      texturePath += "spears.png";      break;
		case FIGURE_LIGHT_HORSE: texturePath += "light_horse.png"; break;
		case FIGURE_TREBUCHET:   texturePath += "trebuchet.png";   break;
		case FIGURE_ELEPHANT:    texturePath += "elephant.png";    break;
		case FIGURE_HEAVY_HORSE: texturePath += "heavy_horse.png"; break;
		case FIGURE_DRAGON:      texturePath += "dragon.png";      break;
		case FIGURE_KING:        texturePath += "king.png";        break;
	}

	_texture = makeTexture(texturePath, 48, 40);
	_quad.setTexture(_texture);
}

MikelepageRuleSet::MikelepageRuleSet(fea::Renderer2D& renderer)
	: RuleSet(renderer, new HexagonalBoard(renderer))
	, _dragonAlive{false, false} // to be set to true on game start
{
}

void MikelepageRuleSet::tick()
{
	_board->tick();
	for(Figure* it : _allActiveFigures)
	{
		_renderer.queue(*it);
	}
}
