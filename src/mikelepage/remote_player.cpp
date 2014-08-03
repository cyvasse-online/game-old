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
#include <server_message.hpp>
#ifdef EMSCRIPTEN
	#include <emscripten.h>
#endif
#include "cyvasse_ws_client.hpp"
#include "rendered_fortress.hpp"
#include "rendered_match.hpp"
#include "rendered_piece.hpp"

namespace mikelepage
{
	using namespace cyvmath;
	using namespace cyvmath::mikelepage;

	using std::placeholders::_1;
	using cyvmath::mikelepage::Coordinate;

	RemotePlayer::RemotePlayer(PlayersColor color, RenderedMatch& match)
		: Player(color, match)
		, m_setupComplete(false)
		, m_match(match) // should probably be considered a workaround
	{
		CyvasseWSClient::instance().handleMessage = std::bind(&RemotePlayer::handleMessage, this, _1);
	}

	void RemotePlayer::removeFortress()
	{
		auto fortress = std::dynamic_pointer_cast<RenderedFortress>(m_fortress);
		m_match.removeTerrain(fortress->getQuad());

		Player::removeFortress();

		m_match.chooseFortressReplacementTile();
	}

	void RemotePlayer::handleMessage(Json::Value msg)
	{
		// TODO: Revise when multiplayer for the native game is implemented
		if(StrToMessage(msg["messageType"].asString()) != Message::GAME_UPDATE)
			throw std::runtime_error("this message should be handled outside the game (message type "
				+ msg["messageType"].asString() + ")");

		Json::Value& data = msg["data"];

		switch(StrToUpdate(msg["update"].asString()))
		{
			case Update::LEAVE_SETUP:
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
					if(!coord && type != PieceType::DRAGON)
						throw std::runtime_error("a " + piece["type"].asString() +
							" piece has no position on the board, this is only allowed for the dragon");

					auto renderedPiece = std::make_shared<RenderedPiece>(type, make_unique(coord), m_color, m_match);

					m_pieceCache.push_back(renderedPiece);
				}

				m_setupComplete = true;
				m_match.tryLeaveSetup();
				break;
			case Update::MOVE_PIECE:
			{
				auto pieceType = StrToPieceType(data["piece type"].asString());
				auto oldPos    = Coordinate::createFromStr(data["old position"].asString());
				auto newPos    = Coordinate::createFromStr(data["new position"].asString());

				if(pieceType == PieceType::UNDEFINED)
					throw std::runtime_error(
						"move of undefined piece " + data["piece type"].asString() + " requested"
					);

				if(!newPos)
					throw std::runtime_error(
						"move to undefined position " + data["new position"].asString() + " requested"
					);

				std::shared_ptr<Piece> piece;

				if(oldPos)
				{
					auto it = m_match.getActivePieces().find(*oldPos);
					if(it == m_match.getActivePieces().end())
						throw std::runtime_error("move of non-existent piece at " + oldPos->toString() + " requested");

					piece = it->second;
				}
				else
				{
					if(pieceType != PieceType::DRAGON)
						throw std::runtime_error("move of piece without position that is not a dragon requested");

					if(!m_dragonAliveInactive)
						throw std::runtime_error("bringing out the dragon requested"
						                         "although it has been brought out already");

					assert(m_inactivePieces.count(PieceType::DRAGON) == 1);
					auto it = m_inactivePieces.find(PieceType::DRAGON);

					piece = it->second;

					assert(piece);
				}

				if(piece->getType() != pieceType)
					throw std::runtime_error(
						"remote client requested move of " + data["piece type"].asString() + ", but there is " +
						PieceTypeToStr(piece->getType()) + " at " + oldPos->toString()
					);

				m_match.tryMovePiece(piece, *newPos);
				break;
			}
			case Update::PROMOTE_PIECE:
			{
				auto from = StrToPieceType(data["from"].asString());
				auto to   = StrToPieceType(data["to"].asString());

				if(!m_fortress)
					throw std::runtime_error("requested promotion of a piece although the fortress is ruined");

				auto piece = m_match.getPieceAt(m_fortress->getCoord());
				if(!piece)
					throw std::runtime_error("requested promotion of a piece although there is no piece on the fortress");

				if(piece->getType() != from)
					throw std::runtime_error("requested promotion of " + PieceTypeToStr(from) + ", but there is a "
						+ PieceTypeToStr(piece->getType()) + " piece in the fortress.");

				switch(from)
				{
					case PieceType::RABBLE:
						if(!(
							to == PieceType::CROSSBOWS ||
							to == PieceType::SPEARS ||
							to == PieceType::LIGHT_HORSE
						   ))
							throw std::runtime_error("requested promotion from rabble to " + PieceTypeToStr(to));

						break;
					case PieceType::CROSSBOWS:
						if(to != PieceType::TREBUCHET)
							throw std::runtime_error("requested promotion from crossbows to " + PieceTypeToStr(to));

						break;
					case PieceType::SPEARS:
						if(to != PieceType::ELEPHANT)
							throw std::runtime_error("requested promotion from spears to " + PieceTypeToStr(to));

						break;
					case PieceType::LIGHT_HORSE:
						if(to != PieceType::HEAVY_HORSE)
							throw std::runtime_error("requested promotion from light horse to " + PieceTypeToStr(to));

						break;
					case PieceType::TREBUCHET:
					case PieceType::ELEPHANT:
					case PieceType::HEAVY_HORSE:
						if(to != PieceType::KING)
							throw std::runtime_error("requested promotion from " + PieceTypeToStr(from) + " to " + PieceTypeToStr(to));
						else if(!m_kingTaken)
							throw std::runtime_error("requested promotion to king when there still is a king");

						break;
					default:
						throw std::runtime_error("requested promotion of unknown piece");
				}

				piece->promoteTo(to);

				break;
			}
			case Update::ADD_FORTRESS_REPLACEMENT_TILE:
			{
				auto coord = Coordinate::createFromStr(data["coordinate"].asString());

				if(!coord || !m_match.addFortressReplacementTile(*coord))
					throw std::runtime_error("fortress replacment tile not valid");

				break;
			}
			case Update::RESIGN:
			default:
				throw std::runtime_error("got a json request with update set to " + msg["update"].asString());
				break;
		}
	}
}
