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

template <int l>
const fea::Color HexagonBoard<l>::tileColors[3] {
	{125,  81,  55},
	{167, 108,  73},
	{191, 140, 109}
};


template <int l>
const fea::Color HexagonBoard<l>::tileColorsDark[3] {
	{ 50,  32,  22},
	{ 67,  43,  29},
	{ 76,  56,  44}
};

template <int l>
const fea::Color HexagonBoard<l>::hoverColor = fea::Color(48, 48, 48, 0);

template <int l>
const typename HexagonBoard<l>::coloringMap HexagonBoard<l>::highlightColors {
	{"red",  {{192, 0, 0, 0}, {0, 64, 64, 0}}},
	{"blue", {{0, 0, 192, 0}, {64, 64, 0, 0}}}
};

template <int l>
typename HexagonBoard<l>::Tile HexagonBoard<l>::noTile()
{
	return std::make_pair(std::unique_ptr<HexagonBoard<l>::Coordinate>(), nullptr);
}

template <int l>
HexagonBoard<l>::HexagonBoard(fea::Renderer2D& renderer, cyvmath::PlayersColor color)
	: m_renderer(renderer)
	, m_upsideDown(color == cyvmath::PlayersColor::WHITE ? false : true)
	, m_hoveredTile(noTile())
	, m_mouseBPressTile(noTile())
{
	const glm::uvec2 windowSize = renderer.getViewport().getSize();

	unsigned padding = std::min(windowSize.x, windowSize.y) / 20;

	m_size = {windowSize.x - padding * 2, windowSize.y - padding * 2};

	if(m_size.x / m_size.y < 4.0f / 3.0f) // wider than 4:3
	{
		m_tileSize.y = m_size.y / static_cast<float>(l * 2 - 1);
		m_tileSize.x = m_tileSize.y / 3.0f * 4.0f;

		m_size.x = m_tileSize.x * (l * 2 - 1);

		m_position = {(windowSize.x - m_size.x) / 2.0f, padding};
	}
	else
	{
		m_tileSize.x = m_size.x / static_cast<float>(l * 2 - 1);
		m_tileSize.y = m_tileSize.x / 4.0f * 3.0f;

		m_size.y = m_tileSize.x * (l * 2 - 1);

		m_position = {padding, (windowSize.y - m_size.y) / 2.0f};
	}

	for(Coordinate c : Hexagon::getAllCoordinates())
	{
		fea::Quad* quad = new fea::Quad(m_tileSize);

		quad->setPosition(getTilePosition(c));
		quad->setColor(getTileColor(c, true));

		// add the tile to the map and vector
		m_tileVec.push_back(quad);
		std::pair<typename TileMap::iterator, bool> res = m_tileMap.insert({c, quad});

		assert(res.second); // assert the insertion was successful
	}
}

template <int l>
HexagonBoard<l>::~HexagonBoard()
{
	for(fea::Quad* it : m_tileVec)
		delete it;
}

template <int l>
glm::uvec2 HexagonBoard<l>::getSize()
{
	return m_size;
}

template <int l>
glm::uvec2 HexagonBoard<l>::getPosition()
{
	return m_position;
}

template <int l>
glm::vec2 HexagonBoard<l>::getTilePosition(Coordinate c)
{
	glm::vec2 ret;

	if(!m_upsideDown) // 'normal' orientation first
	{
		ret.x = m_position.x // padding
			// normal horizontal position offset
			+ m_tileSize.x * c.x()
			// additional horizontal offset due to non-orthogonal y axis
			+ (m_tileSize.x / 2.0f) * (c.y() - (l - 1));

		ret.y = m_position.y // padding
			// inverted normal vertical offset
			// because coordinate y = 0 should be on the bottom
			// but Feather Kit has y = 0 on the top
			+ (m_size.y - (m_tileSize.y * c.y()))
			// to get correct origin point with inverted offsets
			- m_tileSize.y;
	}
	else // upsideDown
	{
		ret.x = m_position.x // padding
			// inverted normal horizontal position offset
			+ (m_size.x - (m_tileSize.x * c.x()))
			// to get correct origin point with inverted offsets
			- m_tileSize.x
			// additional horizontal offset due to non-orthogonal y axis
			- (m_tileSize.x / 2.0f) * (c.y() - (l - 1));

		ret.y = m_position.y // padding
			// twice-inverted -> normal vertical offset
			+ m_tileSize.y * c.y();
	}

	return ret;
}

template <int l>
const glm::vec2& HexagonBoard<l>::getTileSize() const
{
	return m_tileSize;
}

template <int l>
fea::Color HexagonBoard<l>::getTileColor(Coordinate c, bool setup)
{
	int8_t index = (((c.x() - c.y()) % 3) + 3) % 3;

	if(setup && ((!m_upsideDown && c.y() >= (l - 1)) ||
	              (m_upsideDown && c.y() <= (l - 1))))
		return tileColorsDark[index];
	else
		return tileColors[index];
}

template <int l>
std::unique_ptr<typename HexagonBoard<l>::Coordinate> HexagonBoard<l>::getCoordinate(glm::ivec2 tilePosition)
{
	// remove padding - we're just altering a temporary variable
	tilePosition -= glm::ivec2(m_position.x, m_position.y);

	float x, y;

	if(!m_upsideDown) // 'normal' orientation first
	{
		// y = (maximal y) - ('inverted' coord y)
		y = (l * 2 - 1)
			- tilePosition.y / m_tileSize.y;

		x = (
				tilePosition.x
				+ ((l - static_cast<int>(y)) * m_tileSize.x / 2.0f)
				- m_tileSize.x / 2.0f
			)
			/ m_tileSize.x;
	}
	else // upsideDown
	{
		y = tilePosition.y / m_tileSize.y;

		x = (l * 2 - 1)
			- ((
					tilePosition.x
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

template <int l>
fea::Quad* HexagonBoard<l>::getTileAt(Coordinate c)
{
	typename TileMap::iterator it = m_tileMap.find(c);
	if(it == m_tileMap.end())
		return nullptr;

	return it->second;
}

template <int l>
void HexagonBoard<l>::highlightTile(Coordinate coord, const std::string& coloringStr, bool setup)
{
	fea::Quad* quad = getTileAt(coord);
	assert(quad);

	highlight(*quad, getTileColor(coord, setup), coloringStr);
}

template <int l>
void HexagonBoard<l>::resetTileColor(Coordinate coord, bool setup)
{
	fea::Quad* quad = getTileAt(coord);
	assert(quad);

	fea::Color color = getTileColor(coord, setup);
	if(m_hoveredTile.first && *m_hoveredTile.first == coord)
		color += hoverColor;

	quad->setColor(color);
}

template <int l>
void HexagonBoard<l>::resetTileColors(int8_t fromRow, int8_t toRow, bool setup)
{
	assert(fromRow <= toRow);
	for(auto it : m_tileMap)
	{
		if(it.first.y() >= fromRow && it.first.y() <= toRow)
			it.second->setColor(getTileColor(it.first, setup));
	}
}

template <int l>
void HexagonBoard<l>::tick()
{
	for(const fea::Quad* it : m_tileVec)
		m_renderer.queue(*it);
}

template <int l>
void HexagonBoard<l>::onMouseMoved(const fea::Event::MouseMoveEvent& mouseMove)
{
	auto coord = getCoordinate({mouseMove.x, mouseMove.y});

	if(coord) // mouse hovers on tile (*coord)
	{
		fea::Quad* quad = getTileAt(*coord);

		if(!m_hoveredTile.first || *coord != *m_hoveredTile.first)
		{
			// reset color of old hovered tile
			if(m_hoveredTile.first)
				m_hoveredTile.second->setColor(m_hoveredTile.second->getColor() - hoverColor);

			quad->setColor(quad->getColor() + hoverColor);

			m_hoveredTile = std::make_pair(std::move(coord), quad);
		}

        onTileMouseOver(*coord);
	}
	else
	{
		if(m_hoveredTile.first)
		{
			// mouse is outside the board and one tile is still marked with the hover effect
			m_hoveredTile.second->setColor(m_hoveredTile.second->getColor() - hoverColor);
			m_hoveredTile = noTile();
		}

		onMouseMoveOutside(mouseMove);
	}
}

template <int l>
void HexagonBoard<l>::onMouseButtonPressed(const fea::Event::MouseButtonEvent& mouseButton)
{
	if(mouseButton.button != fea::Mouse::Button::LEFT || m_mouseBPressTile.first)
		return;

	auto coord = getCoordinate({mouseButton.x, mouseButton.y});

	if(coord)
	{
		fea::Quad* quad = getTileAt(*coord);

		m_mouseBPressTile = std::make_pair(std::move(coord), quad);
	}
	else
		onClickedOutside(mouseButton);
}

template <int l>
void HexagonBoard<l>::onMouseButtonReleased(const fea::Event::MouseButtonEvent& mouseButton)
{
	if(m_mouseBPressTile.first)
	{
		auto coord = getCoordinate({mouseButton.x, mouseButton.y});

		if(coord && *coord == *m_mouseBPressTile.first)
			onTileClicked(*coord);

		m_mouseBPressTile = noTile();
	}
}
