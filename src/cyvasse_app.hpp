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

#ifndef _CYVASSE_APP_HPP_
#define _CYVASSE_APP_HPP_

#include <fea/rendering/renderer2d.hpp>
#include <fea/structure/application.hpp>
#include <fea/structure/gamestatemachine.hpp>
#include <fea/ui/inputbackend.hpp>
#include <fea/ui/inputhandler.hpp>
#include <fea/ui/window.hpp>
#include <fea/ui/windowbackend.hpp>

class CyvasseApp : public fea::Application
{
	private:
		fea::Window _window;
		fea::InputHandler _input;
		fea::Renderer2D _renderer;
		fea::GameStateMachine _stateMachine;

	protected:
		void setup(const std::vector<std::string>& args) override;
		void loop() override;
		void destroy() override;

	public:
		CyvasseApp();
		virtual ~CyvasseApp() = default;
};

#endif // _CYVASSE_APP_HPP_
