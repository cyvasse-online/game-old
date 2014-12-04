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

#include <cyvmath/mikelepage/fortress.hpp>
#include <cyvws/msg_type.hpp>
#include <cyvws/game_msg_action.hpp>
#include "cyvasse_ws_client.hpp"
#include "rendered_fortress.hpp"
#include "rendered_match.hpp"
#include "rendered_piece.hpp"

using namespace cyvmath;
using namespace cyvmath::mikelepage;
using namespace cyvws;

using std::placeholders::_1;
using cyvmath::mikelepage::Coordinate;

namespace mikelepage
{
	RemotePlayer::RemotePlayer(PlayersColor color, RenderedMatch& match, std::unique_ptr<RenderedFortress> fortress)
		: Player(match, color, std::move(fortress) /*, id */) // TODO
		, m_match(match) // should probably be considered a workaround
	{
		CyvasseWSClient::instance().handleMessage = std::bind(&RemotePlayer::handleMessage, this, _1);
	}

	void RemotePlayer::handleMessage(Json::Value msg)
	{
		// TODO: Revise when multiplayer for the native game is implemented
		if(StrToMsgType(msg["msgType"].asString()) != MsgType::GAME_MSG)
			throw std::runtime_error("this message should be handled outside the game (message type "
				+ msg["msgType"].asString() + ")");

		Json::Value& data = msg["msgData"];

		switch(StrToGameMsgAction(data["action"].asString()))
		{
			case GameMsgAction::SET_OPENING_ARRAY:
				// TODO: Rewrite
				if(data["pieces"].size() != 26)
					throw std::runtime_error("there have to be exactly 26 pieces when leaving setup (got "
						+ std::to_string(data["pieces"].size()) + ")");

				// TODO: check for amounts of pieces of one type
				// TODO: check whether all pieces are on the players side of the board
				for(const Json::Value& piece : data["pieces"])
				{
					auto type = StrToPieceType(piece["type"].asString());
					auto coord = Coordinate::createFromStr(piece["position"].asString());

					if(type == PieceType::UNDEFINED)
						throw std::runtime_error("got unknown piece type " + piece["type"].asString());
					if(!coord)
						throw std::runtime_error("got invalid position " + piece["position"].asString());

					if(type == PieceType::KING)
						m_fortress->setCoord(*coord);

					auto renderedPiece = std::make_shared<RenderedPiece>(type, *coord, m_color, m_match);

					m_pieceCache.push_back(renderedPiece);
				}

				m_match.tryLeaveSetup();
				break;
			case GameMsgAction::SET_IS_READY:
				m_setupComplete = data["param"].asBool();
				break;
			case GameMsgAction::MOVE:
			{
				auto pieceType = StrToPieceType(data["pieceType"].asString());
				auto oldPos    = Coordinate::createFromStr(data["oldPos"].asString());
				auto newPos    = Coordinate::createFromStr(data["newPos"].asString());

				if(pieceType == PieceType::UNDEFINED)
					throw std::runtime_error(
						"move of undefined piece " + data["pieceType"].asString() + " requested"
					);

				if(!newPos)
					throw std::runtime_error(
						"move to undefined position " + data["newPos"].asString() + " requested"
					);

				std::shared_ptr<Piece> piece;

				if(!oldPos)
					throw std::runtime_error("move of piece without position requested");

				auto it = m_match.getActivePieces().find(*oldPos);
				if(it == m_match.getActivePieces().end())
					throw std::runtime_error("move of non-existent piece at " + oldPos->toString() + " requested");

				piece = it->second;

				if(piece->getType() != pieceType)
					throw std::runtime_error(
						"remote client requested move of " + data["pieceType"].asString() + ", but there is " +
						PieceTypeToStr(piece->getType()) + " at " + oldPos->toString()
					);

				m_match.tryMovePiece(piece, *newPos);
				break;
			}
			case GameMsgAction::PROMOTE:
			{
				auto origType = StrToPieceType(data["origType"].asString());
				auto newType  = StrToPieceType(data["newType"].asString());

				if(m_fortress->isRuined)
					throw std::runtime_error("requested promotion of a piece although the fortress is ruined");

				auto piece = m_match.getPieceAt(m_fortress->getCoord());
				if(!piece)
					throw std::runtime_error("requested promotion of a piece although there is no piece on the fortress");

				if(piece->getType() != origType)
					throw std::runtime_error("requested promotion of " + PieceTypeToStr(origType) + ", but there is a "
						+ PieceTypeToStr(piece->getType()) + " piece in the fortress.");

				switch(origType)
				{
					case PieceType::RABBLE:
						if(!(
							newType == PieceType::CROSSBOWS ||
							newType == PieceType::SPEARS ||
							newType == PieceType::LIGHT_HORSE
						   ))
							throw std::runtime_error("requested promotion from rabble to " + PieceTypeToStr(newType));

						break;
					case PieceType::CROSSBOWS:
						if(newType != PieceType::TREBUCHET)
							throw std::runtime_error("requested promotion from crossbows to " + PieceTypeToStr(newType));

						break;
					case PieceType::SPEARS:
						if(newType != PieceType::ELEPHANT)
							throw std::runtime_error("requested promotion from spears to " + PieceTypeToStr(newType));

						break;
					case PieceType::LIGHT_HORSE:
						if(newType != PieceType::HEAVY_HORSE)
							throw std::runtime_error("requested promotion from light horse to " + PieceTypeToStr(newType));

						break;
					case PieceType::TREBUCHET:
					case PieceType::ELEPHANT:
					case PieceType::HEAVY_HORSE:
						if(newType != PieceType::KING)
							throw std::runtime_error("requested promotion from " + PieceTypeToStr(origType) + " to " + PieceTypeToStr(newType));
						else if(!m_kingTaken)
							throw std::runtime_error("requested promotion to king when there still is a king");

						break;
					default:
						throw std::runtime_error("requested promotion of unknown piece");
				}

				piece->promoteTo(newType);

				break;
			}
			case GameMsgAction::RESIGN:
			default:
				throw std::runtime_error("got a json request with action set to " + msg["action"].asString());
				break;
		}
	}
}
