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
#include "rendered_fortress.hpp"
#include "rendered_match.hpp"

using namespace cyvmath;
using namespace cyvws;

namespace mikelepage
{
	LocalPlayer::LocalPlayer(PlayersColor color, RenderedMatch& match, std::unique_ptr<RenderedFortress> fortress)
		: Player(match, color, std::move(fortress) /*, id */) // TODO
		, m_match{match}
	{ }

	void LocalPlayer::onTurnBegin()
	{
		if(m_fortress->isRuined)
			return;

		auto piece = m_match.getPieceAt(m_fortress->getCoord());

		if(piece && piece->getColor() == m_color && piece->getType() != PieceType::KING)
		{
			auto baseTier = piece->getBaseTier();
			PieceType pieceType = piece->getType();
			PieceType promoteToType = PieceType::UNDEFINED;

			if(baseTier == 3)
			{
				if(m_kingTaken)
					promoteToType = PieceType::KING;
			}
			else if(baseTier == 2)
			{
				static const std::map<PieceType, PieceType> nextTierPieces {
					{PieceType::CROSSBOWS, PieceType::TREBUCHET},
					{PieceType::SPEARS, PieceType::ELEPHANT},
					{PieceType::LIGHT_HORSE, PieceType::HEAVY_HORSE}
				};

				if(m_inactivePieces.count(nextTierPieces.at(pieceType)) > 0)
					promoteToType = nextTierPieces.at(pieceType);
			}
			else if(pieceType == PieceType::RABBLE) // can only promote rabble, not the king
			{
				std::set<PieceType> availablePieceTypes;

				for(PieceType type : {PieceType::CROSSBOWS, PieceType::SPEARS, PieceType::LIGHT_HORSE})
				{
					if(m_inactivePieces.count(type) > 0)
						availablePieceTypes.insert(type);
				}

				switch(availablePieceTypes.size())
				{
					case 0:
						break;
					case 1:
						promoteToType = *availablePieceTypes.begin();
						break;
					default:
						m_match.showPromotionPieces(availablePieceTypes);
						break;
				}
			}

			if(promoteToType != PieceType::UNDEFINED)
			{
				piece->promoteTo(promoteToType);
				sendPromotePiece(pieceType, promoteToType);
			}
		}
	}

	void LocalPlayer::sendGameUpdate(cyvws::GameMsgAction action, Json::Value param)
	{
		Json::Value msg;
		msg["msgType"] = "gameMsg";

		auto& msgData = msg["msgData"];
		// TODO: playerID
		msgData["action"] = GameMsgActionToStr(action);
		msgData["param"] = param;

		CyvasseWSClient::instance().send(msg);
	}

	void LocalPlayer::sendLeaveSetup()
	{
		Json::Value param;
		param["pieces"] = Json::Value(Json::arrayValue);

		Json::Value& pieces = param["pieces"];
		for(auto it : m_match.getActivePieces())
		{
			assert(it.second->getColor() == m_color);

			Json::Value piece;
			piece["type"] = PieceTypeToStr(it.second->getType());
			piece["position"] = it.first.toString();

			pieces.append(piece);
		}

		// every piece has to be on the board
		assert(m_inactivePieces.empty());
		assert(m_match.getActivePieces().size() == 26);

		sendGameUpdate(GameMsgAction::SET_OPENING_ARRAY, param);
		sendGameUpdate(GameMsgAction::SET_IS_READY, true);
	}

	void LocalPlayer::sendMovePiece(std::shared_ptr<Piece> piece, Coordinate oldPos)
	{
		assert(piece->getCoord());

		Json::Value param;
		param["pieceType"] = PieceTypeToStr(piece->getType()); // not really relevant, only for debugging
		param["oldPos"]    = oldPos.toString();
		param["newPos"]    = piece->getCoord()->toString();

		sendGameUpdate(GameMsgAction::MOVE, param);
	}

	void LocalPlayer::sendPromotePiece(PieceType origType, PieceType newType)
	{
		assert(origType != PieceType::UNDEFINED);
		assert(newType != PieceType::UNDEFINED);

		Json::Value param;
		param["origType"] = PieceTypeToStr(origType);
		param["newType"]  = PieceTypeToStr(newType);

		sendGameUpdate(GameMsgAction::PROMOTE, param);
	}
}
