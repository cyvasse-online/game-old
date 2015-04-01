/* Copyright 2014 - 2015 Jonas Platte
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
#include <memory>
#include <vector>

#include <fea/rendering/renderer2d.hpp>
#include <fea/rendering/quad.hpp>
#include <fea/ui/event.hpp>

#include <optional.hpp>
#include <cyvasse/hexagon.hpp>
#include <cyvasse/players_color.hpp>

// order of elements here determines order of
// queuing for rendering -> visual z-order
enum class HighlightingId
{
	DIM,
	LAST_MOVE,
	SEL, // selected piece
	PTT, // possible target tiles
	HOVER
};

template<int l>
class HexagonBoard
{
	public:
		typedef typename cyvasse::Hexagon<l> Hexagon;
		typedef typename cyvasse::HexCoordinate<l> HexCoordinate;
		typedef typename std::pair<HexCoordinate, std::shared_ptr<fea::Quad>> Tile;
		typedef typename std::map<HexCoordinate, std::shared_ptr<fea::Quad>> TileMap;
		typedef std::vector<std::shared_ptr<fea::Quad>> QuadVec;
		typedef std::map<HighlightingId, QuadVec> HighlightQuadMap;

		static const fea::Color tileColors[3];

	private:
		static const std::map<HighlightingId, fea::Color> highlightingColors;

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
		QuadVec m_quadVec;

		optional<Tile> m_hoveredTile;

		HighlightQuadMap m_highlightQuads;

		auto getTileColor(HexCoordinate) -> fea::Color;
		auto createHighlightQuad(glm::vec2 pos, HighlightingId) -> std::shared_ptr<fea::Quad>;

	public:
		HexagonBoard(fea::Renderer2D&, cyvasse::PlayersColor);

		// non-copyable
		HexagonBoard(const HexagonBoard&) = delete;
		const HexagonBoard& operator= (const HexagonBoard&) = delete;

		std::function<void(HexCoordinate)> onTileMouseOver;
		std::function<void(HexCoordinate)> onTileClicked;
		std::function<void(const fea::Event::MouseMoveEvent&)> onMouseMoveOutside;
		std::function<void(const fea::Event::MouseButtonEvent&)> onClickedOutside;

		auto getSize() const -> glm::uvec2;
		auto getPosition() const -> glm::uvec2;

		auto getTilePosition(HexCoordinate) const -> glm::vec2;
		auto getTileSize() const -> const glm::vec2&;

		template<class... Args>
		auto getCoordinate(Args&&... args) -> optional<HexCoordinate>;

		auto getTileAt(HexCoordinate) -> std::shared_ptr<fea::Quad>;

		void highlightTile(HexCoordinate, HighlightingId);

		template<class InputIterator>
		void highlightTiles(InputIterator first, InputIterator last, HighlightingId id);

		void clearHighlighting(HighlightingId);

		void tick();

		void onMouseMoved(const fea::Event::MouseMoveEvent&);
		void onMouseButtonReleased(const fea::Event::MouseButtonEvent&);
};

#include "hexagon_board.ipp"

#endif // _HEXAGON_BOARD_HPP_
