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

#include <memory>
#include <json/value.h>
#include <server_message.hpp>

namespace mikelepage
{
	using cyvmath::PlayersColor;
	using cyvmath::mikelepage::Coordinate;
	using cyvmath::mikelepage::Piece;
	using cyvmath::mikelepage::PieceMap;
	using cyvmath::mikelepage::PieceVec;
	using cyvmath::mikelepage::Player;

	class RenderedMatch;

	class LocalPlayer : public Player
	{
		friend RenderedMatch;
		private:
			bool _setupComplete;

			void sendGameUpdate(UpdateType, Json::Value data);

		public:
			LocalPlayer(PlayersColor color, PieceMap& activePieces)
				: Player(color, activePieces)
				, _setupComplete(false)
			{ }

			bool setupComplete() final override
			{ return _setupComplete; }

			void checkSetupComplete()
			{ _setupComplete = Player::setupComplete(); }

			void sendLeaveSetup();
			void sendMovePiece(std::shared_ptr<Piece>, std::unique_ptr<Coordinate> oldPos);
	};
}

#endif // _MIKELEPAGE_LOCAL_PLAYER_HPP_
