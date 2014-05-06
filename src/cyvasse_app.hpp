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

// for fea::Application
#include <featherkit/structure.hpp>
// for fea::Window
#include <featherkit/userinterface.hpp>
// for fea::Renderer2D
#include <featherkit/render2d.hpp>

class CyvasseApp : public fea::Application
{
	private:
		fea::Window _window;
		fea::InputHandler _input;
		fea::Renderer2D _renderer;

	protected:
		void setup(const std::vector<std::string>& args) override;
		void loop() override;
		void destroy() override;

	public:
		CyvasseApp();
};
