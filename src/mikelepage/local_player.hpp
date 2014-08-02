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
#include <cyvmath/mikelepage/fortress.hpp>
#include <server_message.hpp>

namespace mikelepage
{
	using cyvmath::PieceType;
	using cyvmath::PlayersColor;
	using cyvmath::mikelepage::Coordinate;
	using cyvmath::mikelepage::CoordPieceMap;
	using cyvmath::mikelepage::Piece;
	using cyvmath::mikelepage::Player;

	class RenderedMatch;

	class LocalPlayer : public Player
	{
		friend RenderedMatch;
		private:
			bool _setupComplete;

			RenderedMatch& _match;

			void sendGameUpdate(Update, Json::Value data);

		public:
			LocalPlayer(PlayersColor, RenderedMatch&);

			bool setupComplete() final override
			{ return _setupComplete; }

			void checkSetupComplete()
			{ _setupComplete = Player::setupComplete(); }

			void onTurnBegin();

			void removeFortress() final override;

			void sendLeaveSetup();
			void sendMovePiece(std::shared_ptr<Piece>, std::unique_ptr<Coordinate> oldPos);
			void sendPromotePiece(PieceType from, PieceType to);
			void sendAddFortressReplacementTile(Coordinate);
	};
}

#endif // _MIKELEPAGE_LOCAL_PLAYER_HPP_
