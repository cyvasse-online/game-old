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

#ifndef _RENDERED_PIECE_HPP_
#define _RENDERED_PIECE_HPP_

#include <cyvasse/piece.hpp>

#include <map>
#include <string>

#include <fea/rendering/quad.hpp>
#include <fea/rendering/texture.hpp>

template<int> class HexagonBoard;

class RenderedMatch;

class RenderedPiece : public cyvasse::Piece
{
	private:
		HexagonBoard<6>& m_board;

		fea::Quad m_quad;
		fea::Texture m_texture;

	public:
		static fea::Texture makePieceTexture(cyvasse::PlayersColor, cyvasse::PieceType);

		RenderedPiece(cyvasse::PieceType, const HexCoordinate&, cyvasse::PlayersColor, RenderedMatch&);

		fea::Quad* getQuad()
		{ return &m_quad; }

		const glm::vec2& getPosition() const
		{ return m_quad.getPosition(); }

		virtual bool moveTo(const HexCoordinate&, bool setup) final override;

		void setPosition(const glm::vec2& pos)
		{ m_quad.setPosition(pos); }
};

typedef std::vector<std::shared_ptr<RenderedPiece>> RenderedPieceVec;

#endif // _RENDERED_PIECE_HPP_
