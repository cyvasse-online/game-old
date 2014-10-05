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

namespace mikelepage
{
	LocalPlayer::LocalPlayer(PlayersColor color, RenderedMatch& match)
		: Player(color, match)
		, m_setupComplete{false}
		, m_match{match}
	{ }

	void LocalPlayer::onTurnBegin()
	{
		if(m_fortress)
		{
			auto piece = m_match.getPieceAt(m_fortress->getCoord());

			if(piece && piece->getColor() == m_color && piece->getType() != PieceType::KING)
			{
				auto baseTier = piece->getBaseTier();
				PieceType pieceType = piece->getType();
				PieceType promoteToType = PieceType::UNDEFINED;

				if(baseTier == 3)
				{
					if(m_kingTaken)
					{
						promoteToType = PieceType::KING;
						m_kingTaken = false;
					}
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
	}

	void LocalPlayer::removeFortress()
	{
		auto fortress = std::dynamic_pointer_cast<RenderedFortress>(m_fortress);
		m_match.removeFortress(fortress->getQuad());

		Player::removeFortress();
	}

	void LocalPlayer::sendGameUpdate(Update update, Json::Value data)
	{
		Json::Value msg;
		msg["messageType"] = "game update";
		msg["update"] = UpdateToStr(update);
		msg["data"] = data;

		CyvasseWSClient::instance().send(msg);
	}

	void LocalPlayer::sendLeaveSetup()
	{
		Json::Value data;
		data["pieces"] = Json::Value(Json::arrayValue);

		Json::Value& pieces = data["pieces"];
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

		sendGameUpdate(Update::LEAVE_SETUP, data);
	}

	void LocalPlayer::sendMovePiece(std::shared_ptr<Piece> piece, Coordinate oldPos)
	{
		assert(piece->getCoord());

		Json::Value data;
		data["piece type"]   = PieceTypeToStr(piece->getType()); // not really relevant, only for debugging
		data["old position"] = oldPos.toString();
		data["new position"] = piece->getCoord()->toString();

		sendGameUpdate(Update::MOVE_PIECE, data);
	}

	void LocalPlayer::sendPromotePiece(PieceType from, PieceType to)
	{
		assert(from != PieceType::UNDEFINED);
		assert(to != PieceType::UNDEFINED);

		Json::Value data;
		data["from"] = PieceTypeToStr(from);
		data["to"]   = PieceTypeToStr(to);

		sendGameUpdate(Update::PROMOTE_PIECE, data);
	}

	void LocalPlayer::sendAddFortressReplacementTile(Coordinate coord)
	{
		Json::Value data;
		data["coordinate"] = coord.toString();

		sendGameUpdate(Update::ADD_FORTRESS_REPLACEMENT_TILE, data);
	}
}
