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

#include "ruleset.hpp"

#include <featherkit/rendering/renderer2d.hpp>

/** This ruleset was created by Michael Le Page (http://www.mikelepage.com/)

    See http://asoiaf.westeros.org/index.php/topic/58545-complete-cyvasse-rules/
 */
class MikelepageRuleset : public Ruleset
{
	private:
		// non-copyable
		MikelepageRuleset(const MikelepageRuleset&) = delete;
		const MikelepageRuleset& operator= (const MikelepageRuleset&) = delete;

	public:
		MikelepageRuleset(fea::Renderer2D&);

		void setup() override;
		void tick() override;
};

#endif // _MIKELEPAGE_RULESET_HPP_
