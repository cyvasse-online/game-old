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

#include "remote_player.hpp"

#include <cyvmath/mikelepage/common.hpp>
#include <server_message.hpp>
#ifdef EMSCRIPTEN
	#include <emscripten.h>
#endif
#include "cyvasse_ws_client.hpp"
#include "rendered_match.hpp"
#include "rendered_piece.hpp"

namespace mikelepage
{
	using namespace cyvmath;
	using namespace cyvmath::mikelepage;

	using std::placeholders::_1;
	using cyvmath::mikelepage::Coordinate;

	RemotePlayer::RemotePlayer(PlayersColor color, RenderedMatch& match)
		: Player(color, match.getActivePieces())
		, _setupComplete(false)
		, _match(match) // should probably be considered a workaround
	{
		CyvasseWSClient::instance().handleMessage = std::bind(&RemotePlayer::handleMessage, this, _1);
	}

	void RemotePlayer::handleMessage(Json::Value msg)
	{
		// TODO: Revise when multiplayer for the native game is implemented
		if(StrToMessageType(msg["messageType"].asString()) != MESSAGE_GAME_UPDATE)
			throw std::runtime_error("this message should be handled outside the game");

		RenderedPieceVec& allPieces = _match.getAllPieces();

		switch(StrToUpdateType(msg["update"].asString()))
		{
			case UPDATE_LEAVE_SETUP:
				if(msg["data"]["pieces"].size() != 26)
					throw std::runtime_error("there have to be exactly 26 pieces when leaving setup");

				// TODO: check for amounts of pieces of one type
				// TODO: check whether all pieces are on the players side of the board
				for(const Json::Value& piece : msg["data"]["pieces"])
				{
					auto type = StrToPieceType(piece["type"].asString());
					auto coord = Coordinate::createFromStr(piece["position"].asString());

					if(type == PIECE_UNDEFINED)
						throw std::runtime_error("got unknown piece type " + piece["type"].asString());

					auto renderedPiece = std::make_shared<RenderedPiece>(type, make_unique(coord), _color, _match);

					if(coord)
						_activePieces.emplace(*coord, renderedPiece);
					else
						_inactivePieces.push_back(renderedPiece);

					allPieces.push_back(renderedPiece);

					_setupComplete = true;
					_match.tryLeaveSetup();
				}
			case UPDATE_MOVE_PIECE:
			case UPDATE_RESIGN:
			default:
				throw std::runtime_error("got a json request with update set to " + msg["update"].asString());
				break;
		}
	}
}
