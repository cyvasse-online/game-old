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

#ifndef _HEXAGON_BOARD_HPP_
#define _HEXAGON_BOARD_HPP_

#include <algorithm>
#include <map>
#include <vector>
#include <cassert>
#include <cstdlib>
#include <fea/rendering/renderer2d.hpp>
#include <fea/rendering/quad.hpp>
#include <fea/ui/event.hpp>
#include <cyvmath/hexagon.hpp>

template<int l>
class HexagonBoard
{
	public:
		typedef typename cyvmath::Hexagon<l> Hexagon;
		typedef typename Hexagon::Coordinate Coordinate;
		typedef typename std::pair<std::unique_ptr<Coordinate>, fea::Quad*> Tile;
		typedef typename std::map<Coordinate, fea::Quad*> TileMap;
		typedef std::vector<fea::Quad*> TileVec;

	private:
		static const fea::Color highlightColor;

		fea::Renderer2D& _renderer;

		bool _upsideDown;

		// Changing one of these uvec2 variables
		// has very weird effects on the vertical
		// positioning of the board...
		// altough it seems to simply work like this.
		glm::uvec2 _size;
		glm::uvec2 _position;
		glm::vec2 _tileSize;

		TileMap _tileMap;
		TileVec _tileVec;

		Tile _highlightedTile;
		Tile _mouseBPressTile;

		void addHighlightning(Coordinate, bool setup, const fea::Color& cAdd, const fea::Color& cSub);

	public:
		HexagonBoard(fea::Renderer2D&, cyvmath::PlayersColor);
		~HexagonBoard();

		// non-copyable
		HexagonBoard(const HexagonBoard&) = delete;
		const HexagonBoard& operator= (const HexagonBoard&) = delete;

		static Tile noTile();

		std::function<void(Coordinate)> onTileClicked;
		std::function<void(const fea::Event::MouseButtonEvent&)> onClickedOutside;

		glm::uvec2 getSize();
		glm::uvec2 getPosition();

		glm::vec2 getTilePosition(Coordinate);
		const glm::vec2& getTileSize() const;
		fea::Color getTileColor(Coordinate, bool setup);

		std::unique_ptr<Coordinate> getCoordinate(glm::ivec2 tilePosition);

		fea::Quad* getTileAt(Coordinate);

		void highlightTileRed(Coordinate coord, bool setup)
		{ addHighlightning(coord, setup, fea::Color(192, 0, 0, 0), fea::Color(0, 64, 64, 0)); }

		void highlightTileBlue(Coordinate coord, bool setup)
		{ addHighlightning(coord, setup, fea::Color(0, 0, 192, 0), fea::Color(64, 64, 0, 0)); }

		void resetTileColor(Coordinate, bool setup);
		void resetTileColors(int8_t fromRow, int8_t toRow, bool setup = false);

		void tick();

		void onMouseMoved(const fea::Event::MouseMoveEvent&);
		void onMouseButtonPressed(const fea::Event::MouseButtonEvent&);
		void onMouseButtonReleased(const fea::Event::MouseButtonEvent&);
};

#include "hexagon_board.inl"

#endif // _HEXAGON_BOARD_HPP_
