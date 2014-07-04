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

#ifndef _MIKELEPAGE_REMOTE_PLAYER_HPP_
#define _MIKELEPAGE_REMOTE_PLAYER_HPP_

#include <cyvmath/mikelepage/player.hpp>

#include "hexagon_board.hpp"

namespace mikelepage
{
	typedef HexagonBoard<6>::Hexagon Hexagon;

	class MikelepageRuleSet;

	class RemotePlayer : public cyvmath::mikelepage::Player
	{
		private:
			bool _setupComplete;

		public:
			RemotePlayer(cyvmath::PlayersColor color);

			bool setupComplete() final override
			{
				return _setupComplete;
			}
	};
}

#endif // _MIKELEPAGE_REMOTE_PLAYER_HPP_
