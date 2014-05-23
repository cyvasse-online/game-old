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
#include "mikelepage/match.hpp"

#include <fea/rendering/renderer2d.hpp>
//#include <fea/rendering/textsurface.hpp>
#include "hexagon_board.hpp"
#include "mikelepage/piece.hpp"

using namespace cyvmath::mikelepage;

/** This rule set was created by Michael Le Page (http://www.mikelepage.com/)

    See http://asoiaf.westeros.org/index.php/topic/58545-complete-cyvasse-rules/

    This class represents one rendered match, so could be renamed
    MikelepageRuleSetMatch, but the name is long enough as it is
 */
class MikelepageRuleSet : public RuleSet, public Match
{
	public:
		typedef HexagonBoard<6> Board;

	private:
		// non-copyable
		MikelepageRuleSet(const MikelepageRuleSet&) = delete;
		const MikelepageRuleSet& operator= (const MikelepageRuleSet&) = delete;

		class RenderedPiece : public Piece
		{
			public:
				typedef std::vector<RenderedPiece*> RenderedPieceVec;

			private:
				PieceMap& _map;
				Board& _board;

				fea::Texture _texture;
				fea::Quad _quad;

			public:
				RenderedPiece(PieceType, Coordinate*, PlayersColor, PieceMap&, Board&);

				operator const fea::Quad& () const
				{
					return _quad;
				}

				// If setup is true, all "moves" are valid
				void moveTo(Coordinate, bool setup);
		};

		typedef RenderedPiece::RenderedPieceVec RenderedPieceVec;

		Board _board;

		// for rendering
		RenderedPieceVec _allPieces[2];

		//fea::TextSurface _buttonSetupDone;
		fea::Quad _buttonSetupDone;

	public:
		MikelepageRuleSet(fea::Renderer2D&, PlayersColor);
		~MikelepageRuleSet();

		void tick() override;
		void tickSetup();
		void tickPlaying();

		void processEvent(fea::Event&) override;
};

#endif // _MIKELEPAGE_RULESET_HPP_
