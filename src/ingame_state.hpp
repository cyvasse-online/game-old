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

#ifndef _INGAME_STATE_HPP_
#define _INGAME_STATE_HPP_

#include <fea/structure/gamestate.hpp>

#include <functional>
#include <memory>
#include <fea/rendering/quad.hpp>
#include <fea/rendering/renderer2d.hpp>
#include <fea/ui/inputhandler.hpp>

class IngameState : public fea::GameState
{
	private:
		fea::InputHandler& _input;
		fea::Renderer2D& _renderer;

		fea::Quad _background;

	public:
		IngameState(fea::InputHandler&, fea::Renderer2D&);

		// non-copyable
		IngameState(const IngameState&) = delete;
		const IngameState& operator= (const IngameState&) = delete;

		std::function<void(const fea::Event::MouseMoveEvent&)> onMouseMoved;
		std::function<void(const fea::Event::MouseButtonEvent&)> onMouseButtonPressed;
		std::function<void(const fea::Event::MouseButtonEvent&)> onMouseButtonReleased;
		std::function<void(const fea::Event::KeyEvent&)> onKeyPressed;
		std::function<void(const fea::Event::KeyEvent&)> onKeyReleased;

		std::function<void()> tick;

		void setup() override;
		std::string run() override;
};

#endif // _INGAME_STATE_HPP_
