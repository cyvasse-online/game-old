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

#include "cyvasse_ws_client.hpp"
#include "hexagon_board.hpp"

using namespace cyvmath;

namespace mikelepage
{
	void LocalPlayer::sendGameUpdate(UpdateType update, Json::Value data)
	{
		Json::Value msg;
		msg["messageType"] = "game update";
		msg["update"] = UpdateTypeToStr(update);
		msg["data"] = data;

		CyvasseWSClient::instance().send(msg);
	}

	void LocalPlayer::sendLeaveSetup()
	{
		Json::Value data;
		data["pieces"] = Json::Value(Json::arrayValue);

		Json::Value& pieces = data["pieces"];
		for(auto it : _activePieces)
		{
			assert(it.second->getColor() == _color);

			Json::Value piece;
			piece["type"] = PieceTypeToStr(it.second->getType());
			piece["position"] = it.first.toString();

			pieces.append(piece);
		}

		// after setup, if there is an inactive piece, it has to be
		// the dragon; otherwise _inactivePieces has to be empty
		assert(_inactivePieces.size() <= 1);
		assert(_inactivePieces.front()->getType() == PIECE_DRAGON);

		if(_inactivePieces.size() > 0)
		{
			Json::Value piece;
			piece["type"] = PieceTypeToStr(_inactivePieces.front()->getType());
			piece["position"] = Json::Value(); // null

			pieces.append(piece);
		}

		sendGameUpdate(UPDATE_LEAVE_SETUP, data);
	}

	void LocalPlayer::sendMovePiece(std::shared_ptr<Piece> piece, std::unique_ptr<Coordinate> oldPos)
	{
		assert(piece->getCoord());

		Json::Value data;
		data["piece type"]   = PieceTypeToStr(piece->getType()); // not really relevant, only for debugging
		data["old position"] = oldPos ? oldPos->toString() : Json::Value();
		data["new position"] = piece->getCoord()->toString();

		sendGameUpdate(UPDATE_MOVE_PIECE, data);
	}
}
