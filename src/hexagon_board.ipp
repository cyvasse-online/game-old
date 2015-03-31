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
	{191, 140, 109},
	{125,  81,  55},
	{167, 108,  73}
};

template <int l>
const std::map<HighlightingId, fea::Color> HexagonBoard<l>::highlightingColors {
	{HighlightingId::DIM,       {  0,   0,   0, 143}},
	{HighlightingId::LAST_MOVE, {  0, 143, 255,  95}},
	{HighlightingId::SEL,       {255,   0,   0, 143}},
	{HighlightingId::PTT,       {  0,   0, 255,  95}},
	{HighlightingId::HOVER,     {255, 255, 255,  63}}
};

template <int l>
fea::Color HexagonBoard<l>::getTileColor(Coordinate c)
{
	auto i = (((c.x() - c.y()) % 3) + 3) % 3;
	return tileColors[i];
}

template <int l>
std::shared_ptr<fea::Quad> HexagonBoard<l>::createHighlightQuad(glm::vec2 pos, HighlightingId id)
{
	auto ret = std::make_shared<fea::Quad>(getTileSize());

	ret->setPosition(pos);
	ret->setColor(highlightingColors.at(id));

	return ret;
}

template <int l>
HexagonBoard<l>::HexagonBoard(fea::Renderer2D& renderer, cyvasse::PlayersColor color)
	: m_renderer(renderer)
	, m_upsideDown(color == cyvasse::PlayersColor::WHITE ? false : true)
{
	const glm::uvec2 windowSize = renderer.getViewport().getSize();

	unsigned padding = std::min(windowSize.x, windowSize.y) / 40;

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

	std::vector<Coordinate> tmpVec;
	tmpVec.reserve(Hexagon::tileCount + (Hexagon::edgeLength * 2) - 1);

	for(Coordinate c : Hexagon::allCoordinates)
	{
		auto quad = std::make_shared<fea::Quad>(m_tileSize);

		quad->setPosition(getTilePosition(c));
		quad->setColor(getTileColor(c));

		if((!m_upsideDown && c.y() >= (l - 1)) ||
		   (m_upsideDown && c.y() <= (l - 1)))
			tmpVec.push_back(c);

		// add the tile to the map and vector
		m_quadVec.push_back(quad);
		auto res = m_tileMap.emplace(c, quad);

		assert(res.second); // assert the insertion was successful
	}

	highlightTiles(tmpVec.begin(), tmpVec.end(), HighlightingId::DIM);
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

template<int l>
template<class... Args>
optional<typename HexagonBoard<l>::Coordinate> HexagonBoard<l>::getCoordinate(Args&&... args)
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
		return nullopt;

	return Coordinate::create(static_cast<int>(x), static_cast<int>(y));
}

template <int l>
std::shared_ptr<fea::Quad> HexagonBoard<l>::getTileAt(Coordinate c)
{
	typename TileMap::iterator it = m_tileMap.find(c);
	if(it == m_tileMap.end())
		return nullptr;

	return it->second;
}

template <int l>
void HexagonBoard<l>::highlightTile(Coordinate coord, HighlightingId id)
{
	auto res = m_highlightQuads.emplace(id, QuadVec());
	auto& quadVec = res.first->second;

	// the check could maybe be removed
	if(!res.second)
		quadVec.clear();

	quadVec.push_back(createHighlightQuad(getTilePosition(coord), id));
}

template <int l>
template <class InputIterator>
void HexagonBoard<l>::highlightTiles(InputIterator first, InputIterator last, HighlightingId id)
{
	static_assert(
		std::is_convertible<typename std::iterator_traits<InputIterator>::value_type, Coordinate>::value,
		"The iterators first and last have to be convertible to Coordinate"
	);

	auto res = m_highlightQuads.emplace(id, QuadVec());
	auto& quadVec = res.first->second;

	// the check could maybe be removed
	if(!res.second)
		quadVec.clear();

	while(first != last)
	{
		quadVec.push_back(createHighlightQuad(getTilePosition(*first), id));
		++first;
	}
}

template <int l>
void HexagonBoard<l>::clearHighlighting(HighlightingId id)
{
	auto res = m_highlightQuads.find(id);
	if(res != m_highlightQuads.end())
		res->second.clear();
}

template <int l>
void HexagonBoard<l>::tick()
{
	for(auto&& it : m_quadVec)
		m_renderer.queue(*it);

	for(auto&& vecIt : m_highlightQuads)
		for(auto&& quadIt : vecIt.second)
			m_renderer.queue(*quadIt);
}

template <int l>
void HexagonBoard<l>::onMouseMoved(const fea::Event::MouseMoveEvent& mouseMove)
{
	auto coord = getCoordinate(mouseMove.x, mouseMove.y);

	if(coord) // mouse hovers on tile (*coord)
	{
		auto quad = getTileAt(*coord);

		if(!m_hoveredTile || *coord != m_hoveredTile->first)
		{
			highlightTile(*coord, HighlightingId::HOVER);
			m_hoveredTile = Tile(*coord, quad);

			onTileMouseOver(*coord);
		}
	}
	else
	{
		if(m_hoveredTile)
		{
			// mouse is outside the board and one tile is still marked with the hover effect
			clearHighlighting(HighlightingId::HOVER);
			m_hoveredTile = nullopt;
		}

		onMouseMoveOutside(mouseMove);
	}
}

template <int l>
void HexagonBoard<l>::onMouseButtonReleased(const fea::Event::MouseButtonEvent& mouseButton)
{
	auto coord = getCoordinate(mouseButton.x, mouseButton.y);

	if(coord)
		onTileClicked(*coord);
	else
		onClickedOutside(mouseButton);
}
