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

#include <map>
#include <memory>
#include <stdexcept>
#include <cyvmath/mikelepage/fortress.hpp>
#include <cyvws/msg_type.hpp>
#include <cyvws/game_msg_action.hpp>
#include "cyvasse_ws_client.hpp"
#include "rendered_fortress.hpp"
#include "rendered_match.hpp"
#include "rendered_piece.hpp"

using namespace std;
using std::placeholders::_1;

using namespace cyvmath;
using namespace cyvmath::mikelepage;
using namespace cyvws;

namespace mikelepage
{
	using HexCoordinate = Hexagon<6>::Coordinate;

	RemotePlayer::RemotePlayer(PlayersColor color, RenderedMatch& match, unique_ptr<RenderedFortress> fortress)
		: Player(match, color, move(fortress) /*, id */) // TODO
		, m_match(match) // should probably be considered a workaround
	{
		CyvasseWSClient::instance().handleMessage = bind(&RemotePlayer::handleMessage, this, _1);
	}

	void RemotePlayer::handleMessage(Json::Value msg)
	{
		// TODO: Revise when multiplayer for the native game is implemented
		if(StrToMsgType(msg["msgType"].asString()) != MsgType::GAME_MSG)
			throw runtime_error("this message should be handled outside the game (message type "
				+ msg["msgType"].asString() + ")");

		auto& msgData = msg["msgData"];
		auto& param = msgData["param"];

		switch(StrToGameMsgAction(msgData["action"].asString()))
		{
			case GameMsgAction::SET_OPENING_ARRAY:
				if(param.size() != 10)
				{
					throw runtime_error("There have to be exactly 10 piece types in the opening array (got "
						+ to_string(param.size()) + ")");
				}

				// TODO: Maybe move this data to somewhere else
				static const map<PieceType, uint8_t> pieceTypeNum {
					{PieceType::MOUNTAINS, 6},
					{PieceType::RABBLE, 6},
					{PieceType::KING, 1},
					{PieceType::CROSSBOWS, 2},
					{PieceType::SPEARS, 2},
					{PieceType::LIGHT_HORSE, 2},
					{PieceType::TREBUCHET, 2},
					{PieceType::ELEPHANT, 2},
					{PieceType::HEAVY_HORSE, 2},
					{PieceType::DRAGON, 1}
				};

				for(const auto& it : pieceTypeNum)
				{
					const auto& typeStr = PieceTypeToStr(it.first);

					auto& coords = param[typeStr];
					if(coords.size() != it.second)
					{
						throw runtime_error("There have to be exactly " + to_string(it.second) + ' ' + typeStr
							+ " pieces in the opening array (got " + to_string(coords.size()) + ")");
					}

					// TODO: check whether all pieces are on the players side of the board
					for(const auto& coordVal : coords)
					{
						auto coord = HexCoordinate::createFromStr(coordVal.asString());

						if(!coord)
						{
							throw runtime_error("got invalid coordinate " + coordVal.asString()
								+ " for a " + typeStr + " piece");
						}

						if(it.first == PieceType::KING)
							m_fortress->setCoord(*coord);

						m_pieceCache.push_back(make_shared<RenderedPiece>(it.first, *coord, m_color, m_match));
					}
				}

				m_match.tryLeaveSetup();
				break;
			case GameMsgAction::SET_IS_READY:
				m_setupComplete = param.asBool();
				break;
			case GameMsgAction::MOVE:
			{
				auto pieceType = StrToPieceType(param["pieceType"].asString());
				auto oldPos    = HexCoordinate::createFromStr(param["oldPos"].asString());
				auto newPos    = HexCoordinate::createFromStr(param["newPos"].asString());

				if(pieceType == PieceType::UNDEFINED)
					throw runtime_error(
						"move of undefined piece " + param["pieceType"].asString() + " requested"
					);

				if(!newPos)
					throw runtime_error(
						"move to undefined position " + param["newPos"].asString() + " requested"
					);

				shared_ptr<cyvmath::mikelepage::Piece> piece;

				if(!oldPos)
					throw runtime_error("move of piece without position requested");

				auto it = m_match.getActivePieces().find(*oldPos);
				if(it == m_match.getActivePieces().end())
					throw runtime_error("move of non-existent piece at " + oldPos->toString() + " requested");

				piece = it->second;

				if(piece->getType() != pieceType)
					throw runtime_error(
						"remote client requested move of " + param["pieceType"].asString() + ", but there is " +
						PieceTypeToStr(piece->getType()) + " at " + oldPos->toString()
					);

				m_match.tryMovePiece(piece, *newPos);
				break;
			}
			case GameMsgAction::PROMOTE:
			{
				auto origType = StrToPieceType(param["origType"].asString());
				auto newType  = StrToPieceType(param["newType"].asString());

				if(m_fortress->isRuined)
					throw runtime_error("requested promotion of a piece although the fortress is ruined");

				auto piece = m_match.getPieceAt(m_fortress->getCoord());
				if(!piece)
					throw runtime_error("requested promotion of a piece although there is no piece on the fortress");

				if(piece->getType() != origType)
					throw runtime_error("requested promotion of " + PieceTypeToStr(origType) + ", but there is a "
						+ PieceTypeToStr(piece->getType()) + " piece in the fortress.");

				switch(origType)
				{
					case PieceType::RABBLE:
						if(!(
							newType == PieceType::CROSSBOWS ||
							newType == PieceType::SPEARS ||
							newType == PieceType::LIGHT_HORSE
						   ))
							throw runtime_error("requested promotion from rabble to " + PieceTypeToStr(newType));

						break;
					case PieceType::CROSSBOWS:
						if(newType != PieceType::TREBUCHET)
							throw runtime_error("requested promotion from crossbows to " + PieceTypeToStr(newType));

						break;
					case PieceType::SPEARS:
						if(newType != PieceType::ELEPHANT)
							throw runtime_error("requested promotion from spears to " + PieceTypeToStr(newType));

						break;
					case PieceType::LIGHT_HORSE:
						if(newType != PieceType::HEAVY_HORSE)
							throw runtime_error("requested promotion from light horse to " + PieceTypeToStr(newType));

						break;
					case PieceType::TREBUCHET:
					case PieceType::ELEPHANT:
					case PieceType::HEAVY_HORSE:
						if(newType != PieceType::KING)
							throw runtime_error("requested promotion from " + PieceTypeToStr(origType) + " to " + PieceTypeToStr(newType));
						else if(!m_kingTaken)
							throw runtime_error("requested promotion to king when there still is a king");

						break;
					default:
						throw runtime_error("requested promotion of unknown piece");
				}

				piece->promoteTo(newType);

				break;
			}
			case GameMsgAction::RESIGN:
			default:
				throw runtime_error("got a json request with action set to " + msg["action"].asString());
				break;
		}
	}
}
