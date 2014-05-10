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

#ifndef _MIKELEPAGE_RULESET_HPP_
#define _MIKELEPAGE_RULESET_HPP_

#include "rule_set.hpp"

#include <featherkit/rendering/renderer2d.hpp>
#include "boards/hexagonal_board.hpp"

/** This rule set was created by Michael Le Page (http://www.mikelepage.com/)

    See http://asoiaf.westeros.org/index.php/topic/58545-complete-cyvasse-rules/
 */
class MikelepageRuleSet : public RuleSet
{
	public:
		enum FigureType
		{
			FIGURE_MOUNTAIN,
			FIGURE_RABBLE,
			FIGURE_CROSSBOWS,
			FIGURE_SPEARS,
			FIGURE_LIGHT_HORSE,
			FIGURE_TREBUCHET,
			FIGURE_ELEPHANT,
			FIGURE_HEAVY_HORSE,
			FIGURE_DRAGON,
			FIGURE_KING
		};

		enum PlayersColor
		{
			PLAYER_WHITE,
			PLAYER_BLACK
		};

	private:
		// non-copyable
		MikelepageRuleSet(const MikelepageRuleSet&) = delete;
		const MikelepageRuleSet& operator= (const MikelepageRuleSet&) = delete;

		class Figure
		{
			private:
				FigureType _type;

				fea::Texture _texture;
				fea::Quad _quad;

			public:
				Figure(FigureType, PlayersColor);

				operator fea::Quad& ()
				{
					return _quad;
				}
		};

		typedef std::unordered_map<HexagonalBoard::Coordinate, Figure*, std::hash<int>> figureMap;
		typedef std::vector<Figure*> figureVec;

		// the following variables are arrays because they exist once for each player

		// figures which are active (on the board) can be found by their coordinate
		figureMap _activeFigures[2];
		figureVec _inactiveFigures[2];

		// the dragon is the only figure that can be inactive but alive (after setup)
		bool _dragonAlive[2];

		// the same as both _activeFigures maps together
		// for rendering
		figureVec _allActiveFigures;

	public:
		MikelepageRuleSet(fea::Renderer2D&);

		void setup() override;
		void tick() override;
};

#endif // _MIKELEPAGE_RULESET_HPP_
