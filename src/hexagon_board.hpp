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
		typedef typename cyvmath::Hexagon<l> Hexagon;
		typedef typename Hexagon::Coordinate Coordinate;
		typedef typename std::pair<Coordinate, std::shared_ptr<fea::Quad>> Tile;
		typedef typename std::map<Coordinate, std::shared_ptr<fea::Quad>> TileMap;
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

		// TODO: change to std::optional [C++14]
		std::unique_ptr<Tile> m_hoveredTile;
		std::unique_ptr<Tile> m_mouseBPressTile;

		HighlightQuadMap m_highlightQuads;

		fea::Color getTileColor(Coordinate);
		std::shared_ptr<fea::Quad> createHighlightQuad(glm::vec2 pos, HighlightingId);

	public:
		HexagonBoard(fea::Renderer2D&, cyvmath::PlayersColor);

		// non-copyable
		HexagonBoard(const HexagonBoard&) = delete;
		const HexagonBoard& operator= (const HexagonBoard&) = delete;

		std::function<void(Coordinate)> onTileMouseOver;
		std::function<void(Coordinate)> onTileClicked;
		std::function<void(const fea::Event::MouseMoveEvent&)> onMouseMoveOutside;
		std::function<void(const fea::Event::MouseButtonEvent&)> onClickedOutside;

		glm::uvec2 getSize();
		glm::uvec2 getPosition();

		glm::vec2 getTilePosition(Coordinate);
		const glm::vec2& getTileSize() const;

		template<class... Args>
		std::unique_ptr<Coordinate> getCoordinate(Args&&... args)
		{
			// remove padding
			glm::uvec2 tilePos = glm::uvec2(std::forward<Args>(args)...) - m_position;

			float x, y;

			if(!m_upsideDown) // 'normal' orientation first
			{
				// y = (maximal y) - ('inverted' coord y)
				y = (l * 2 - 1)
					- tilePos.y / m_tileSize.y;

				x = (
						tilePos.x
						+ ((l - static_cast<int>(y)) * m_tileSize.x / 2.0f)
						- m_tileSize.x / 2.0f
					)
					/ m_tileSize.x;
			}
			else // upsideDown
			{
				y = tilePos.y / m_tileSize.y;

				x = (l * 2 - 1)
					- ((
							tilePos.x
							- ((l - static_cast<int>(y)) * m_tileSize.x / 2.0f)
							+ m_tileSize.x / 2.0f
						)
						/ m_tileSize.x
					);
			}

			if(x < 0.0f || y < 0.0f)
				return nullptr;

			return Coordinate::create(static_cast<int>(x), static_cast<int>(y));
		}

		std::shared_ptr<fea::Quad> getTileAt(Coordinate);

		void highlightTile(Coordinate, HighlightingId, bool removeExisting = true);
		void highlightTile(glm::vec2, HighlightingId, bool removeExisting = true);

		template<class InputIterator>
		void highlightTiles(InputIterator first, InputIterator last, HighlightingId id, bool removeExisting = true)
		{
			static_assert(
				std::is_convertible<typename std::iterator_traits<InputIterator>::value_type, Coordinate>::value,
				"The iterators first and last have to be convertible to Coordinate"
			);

			auto res = m_highlightQuads.emplace(id, QuadVec());
			auto& quadVec = res.first->second;

			if(removeExisting && !res.second)
				quadVec.clear();

			while(first != last)
			{
				quadVec.push_back(createHighlightQuad(getTilePosition(*first), id));
				++first;
			}
		}

		void clearHighlighting(HighlightingId);

		void queueTileRendering();
		void queueHighlightingRendering();

		void onMouseMoved(const fea::Event::MouseMoveEvent&);
		void onMouseButtonPressed(const fea::Event::MouseButtonEvent&);
		void onMouseButtonReleased(const fea::Event::MouseButtonEvent&);
};

#include "hexagon_board.inl"

#endif // _HEXAGON_BOARD_HPP_
