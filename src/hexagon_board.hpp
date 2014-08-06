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

#include <map>
#include <vector>
#include <fea/rendering/renderer2d.hpp>
#include <fea/rendering/quad.hpp>
#include <fea/ui/event.hpp>
#include <cyvmath/hexagon.hpp>
#include <cyvmath/players_color.hpp>

template<int l>
class HexagonBoard
{
	public:
		typedef typename cyvmath::Hexagon<l> Hexagon;
		typedef typename Hexagon::Coordinate Coordinate;
		typedef typename std::pair<std::unique_ptr<Coordinate>, fea::Quad*> Tile;
		typedef typename std::map<Coordinate, fea::Quad*> TileMap;
		typedef std::vector<fea::Quad*> TileVec;
		typedef std::map<std::string, std::pair<fea::Color, fea::Color>> coloringMap;

		static const fea::Color tileColors[3];
		static const fea::Color tileColorsDark[3];
		static const fea::Color hoverColor;
		static const coloringMap highlightColors;

	private:
		fea::Renderer2D& m_renderer;

		bool m_upsideDown;

		// Changing one of these uvec2 variables
		// has very weird effects on the vertical
		// positioning of the board...
		// altough it seems to simply work like this.
		glm::uvec2 m_size;
		glm::uvec2 m_position;
		glm::vec2 m_tileSize;

		TileMap m_tileMap;
		TileVec m_tileVec;

		Tile m_hoveredTile;
		Tile m_mouseBPressTile;

		static void highlight(fea::Quad& tile, const fea::Color& base,
		                      const std::pair<fea::Color, fea::Color>& coloring)
		{ tile.setColor(base + coloring.first - coloring.second); }

	public:
		HexagonBoard(fea::Renderer2D&, cyvmath::PlayersColor);
		~HexagonBoard();

		// non-copyable
		HexagonBoard(const HexagonBoard&) = delete;
		const HexagonBoard& operator= (const HexagonBoard&) = delete;

		static Tile noTile();

		static void highlight(fea::Quad& tile, const fea::Color& base, const std::string& coloringStr)
		{ highlight(tile, base, highlightColors.at(coloringStr)); }

		std::function<void(Coordinate)> onTileMouseOver;
		std::function<void(Coordinate)> onTileClicked;
		std::function<void(const fea::Event::MouseMoveEvent&)> onMouseMoveOutside;
		std::function<void(const fea::Event::MouseButtonEvent&)> onClickedOutside;

		glm::uvec2 getSize();
		glm::uvec2 getPosition();

		glm::vec2 getTilePosition(Coordinate);
		const glm::vec2& getTileSize() const;
		fea::Color getTileColor(Coordinate, bool setup);

		std::unique_ptr<Coordinate> getCoordinate(glm::ivec2 tilePosition);

		fea::Quad* getTileAt(Coordinate);

		void highlightTile(Coordinate coord, const std::string& coloringStr, bool setup);

		template<class InputIterator>
		void highlightTiles(InputIterator first, InputIterator last, const std::string& coloringStr, const std::string& tileSetId)
		{
			// TODO
		}

		void resetTileColor(Coordinate, bool setup);
		void resetTileColors(int8_t fromRow, int8_t toRow, bool setup = false);

		void tick();

		void onMouseMoved(const fea::Event::MouseMoveEvent&);
		void onMouseButtonPressed(const fea::Event::MouseButtonEvent&);
		void onMouseButtonReleased(const fea::Event::MouseButtonEvent&);
};

#include "hexagon_board.inl"

#endif // _HEXAGON_BOARD_HPP_
