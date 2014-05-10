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

#ifndef _RULE_SET_HPP_
#define _RULE_SET_HPP_

#include <featherkit/rendering/renderer2d.hpp>
#include "board.hpp"

class RuleSet
{
	private:
		// non-copyable
		RuleSet(const RuleSet&) = delete;
		const RuleSet& operator= (const RuleSet&) = delete;

	protected:
		fea::Renderer2D& _renderer;

		std::unique_ptr<Board> _board;

	public:
		RuleSet(fea::Renderer2D&, Board*);

		virtual void setup() = 0;
		virtual void tick() = 0;
};

#endif // _RULE_SET_HPP_
