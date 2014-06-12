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

#include "local_player.hpp"

using namespace cyvmath::mikelepage;

namespace mikelepage
{
	LocalPlayer::LocalPlayer(PlayersColor color)
		: Player(color)
		, _setupComplete(false)
	{
	}

	void LocalPlayer::checkSetupComplete()
	{
		auto outsideOwnSide = (_color == PLAYER_WHITE) ? [](int8_t y) { return y >= (Hexagon::edgeLength - 1); }
		                                               : [](int8_t y) { return y <= (Hexagon::edgeLength - 1); };

		for(auto& it : _activePieces)
		{
			if(outsideOwnSide(it.first->y()))
			{
				_setupComplete = false;
				return;
			}
		}

		_setupComplete = true;
	}
}
