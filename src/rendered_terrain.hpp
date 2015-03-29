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

#ifndef _RENDERED_TERRAIN_HPP_
#define _RENDERED_TERRAIN_HPP_

#include <cyvasse/terrain.hpp>
#include <fea/rendering/quad.hpp>

template<int> class HexagonBoard;

using cyvasse::TerrainMap;

class RenderedTerrain : public cyvasse::Terrain
{
	private:
		HexagonBoard<6>& m_board;
		TerrainMap& m_terrainMap;

		fea::Quad m_quad;
		fea::Texture m_texture;

	public:
		RenderedTerrain(cyvasse::TerrainType, cyvasse::Coordinate, HexagonBoard<6>&, TerrainMap&);

		fea::Quad* getQuad()
		{ return &m_quad; }

		void setCoord(cyvasse::Coordinate) final override;
};

#endif // _RENDERED_TERRAIN_HPP_
