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
#include <cyvws/common.hpp>
#include <cyvws/msg.hpp>
#include <cyvws/game_msg.hpp>
#include <cyvws/json_game_msg.hpp>
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
		if (msg[MSG_TYPE].asString() != MsgType::GAME_MSG)
			throw runtime_error("this message should be handled outside the game (message type "
				+ msg[MSG_TYPE].asString() + ")");

		const auto& msgData = msg[MSG_DATA];
		const auto& param = msgData[PARAM];

		const auto& action = msgData[ACTION].asString();

		if (action == GameMsgAction::SET_OPENING_ARRAY)
		{
			const auto& pieces = json::pieceMap(param);
			evalOpeningArray(pieces);

			for (const auto& it : pieces)
			{
				for (const auto& coord : it.second)
				{
					// TODO: Move this somewhere else (probably cyvmath)
					if (it.first == PieceType::KING)
						m_fortress->setCoord(coord);

					m_pieceCache.push_back(make_shared<RenderedPiece>(it.first, coord, m_color, m_match));
				}
			}

			m_setupComplete = true;
			m_match.tryLeaveSetup();
		}
		else if (action == GameMsgAction::SET_IS_READY)
		{ }
		else if (action == GameMsgAction::MOVE)
		{
			auto movement = json::movement(param);

			if (movement.pieceType == PieceType::UNDEFINED)
				throw runtime_error("move of undefined piece " + PieceTypeToStr(movement.pieceType) + " requested");

			auto it = m_match.getActivePieces().find(movement.oldPos);
			if (it == m_match.getActivePieces().end())
				throw runtime_error("move of non-existent piece at " + movement.oldPos.toString() + " requested");

			auto piece = it->second;

			if (piece->getType() != movement.pieceType)
				throw runtime_error(
					"remote client requested move of " + PieceTypeToStr(movement.pieceType) + ", but there is " +
					PieceTypeToStr(piece->getType()) + " at " + movement.oldPos.toString()
				);

			m_match.tryMovePiece(piece, movement.newPos);
		}
		else if (action == GameMsgAction::MOVE_CAPTURE)
		{
			auto movement = json::moveCapture(param);

			if (movement.atkPT == PieceType::UNDEFINED)
				throw runtime_error("move of undefined piece " + PieceTypeToStr(movement.atkPT) + " requested");
			if (movement.defPT == PieceType::UNDEFINED)
				throw runtime_error("capture of undefined piece " + PieceTypeToStr(movement.atkPT) + " requested");

			auto it = m_match.getActivePieces().find(movement.oldPos);

			if (it == m_match.getActivePieces().end())
				throw runtime_error("move of non-existent piece at " + movement.oldPos.toString() + " requested");
			if (it->second->getType() != movement.atkPT)
				throw runtime_error(
					"move of " + PieceTypeToStr(movement.atkPT) + " requested, but there is " +
					PieceTypeToStr(it->second->getType()) + " at " + movement.oldPos.toString()
				);

			auto piece = it->second;

			it = m_match.getActivePieces().find(movement.defPiecePos);

			if (it == m_match.getActivePieces().end())
				throw runtime_error("capture of non-existent piece at " + movement.defPiecePos.toString() + " requested");
			if (it->second->getType() != movement.defPT)
				throw runtime_error(
					"capture of " + PieceTypeToStr(movement.defPT) + " requested, but there is " +
					PieceTypeToStr(it->second->getType()) + " at " + movement.defPiecePos.toString()
				);

			m_match.tryMovePiece(piece, movement.newPos);
		}
		else if (action == GameMsgAction::PROMOTE)
		{
			auto promotion = json::promotion(param);

			if (m_fortress->isRuined)
				throw runtime_error("requested promotion of a piece although the fortress is ruined");

			auto piece = m_match.getPieceAt(m_fortress->getCoord());
			if (!piece)
				throw runtime_error("requested promotion of a piece although there is no piece on the fortress");

			if (piece->getType() != promotion.origType)
				throw runtime_error("requested promotion of " + PieceTypeToStr(promotion.origType) + ", but there is a "
					+ PieceTypeToStr(piece->getType()) + " piece in the fortress.");

			switch(promotion.origType)
			{
				case PieceType::RABBLE:
					if (!(promotion.newType == PieceType::CROSSBOWS ||
						promotion.newType == PieceType::SPEARS ||
						promotion.newType == PieceType::LIGHT_HORSE))
						throw runtime_error("requested promotion from rabble to " + PieceTypeToStr(promotion.newType));

					break;
				case PieceType::CROSSBOWS:
					if (promotion.newType != PieceType::TREBUCHET)
						throw runtime_error("requested promotion from crossbows to " + PieceTypeToStr(promotion.newType));

					break;
				case PieceType::SPEARS:
					if (promotion.newType != PieceType::ELEPHANT)
						throw runtime_error("requested promotion from spears to " + PieceTypeToStr(promotion.newType));

					break;
				case PieceType::LIGHT_HORSE:
					if (promotion.newType != PieceType::HEAVY_HORSE)
						throw runtime_error("requested promotion from light horse to " + PieceTypeToStr(promotion.newType));

					break;
				case PieceType::TREBUCHET:
				case PieceType::ELEPHANT:
				case PieceType::HEAVY_HORSE:
					if (promotion.newType != PieceType::KING)
						throw runtime_error("requested promotion from " + PieceTypeToStr(promotion.origType) +
							" to " + PieceTypeToStr(promotion.newType));
					else if (!m_kingTaken)
						throw runtime_error("requested promotion to king when there still is a king");

					break;
				default:
					throw runtime_error("requested promotion of unknown piece");
			}

			piece->promoteTo(promotion.newType);
		}
		//else if (action == GameMsgAction::RESIGN)
		else
			throw runtime_error("got a json request with action set to " + msg[ACTION].asString());
	}
}
