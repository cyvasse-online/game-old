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

#ifndef _MIKELEPAGE_LOCAL_PLAYER_HPP_
#define _MIKELEPAGE_LOCAL_PLAYER_HPP_

#include <cyvmath/mikelepage/player.hpp>
#include <cyvws/game_msg.hpp>
#include <memory>
#include <json/value.h>
#include "rendered_fortress.hpp"

namespace mikelepage
{
	class RenderedMatch;

	class LocalPlayer : public cyvmath::mikelepage::Player
	{
		friend RenderedMatch;
		private:
			bool m_setupComplete = false;

			RenderedMatch& m_match;

		public:
			LocalPlayer(cyvmath::PlayersColor, RenderedMatch&, std::unique_ptr<RenderedFortress> = {});
			virtual ~LocalPlayer() = default;

			bool setupComplete() const final override
			{ return m_setupComplete; }

			void checkSetupComplete()
			{ m_setupComplete = Player::setupComplete(); }

			void onTurnBegin();
	};
}

#endif // _MIKELEPAGE_LOCAL_PLAYER_HPP_
