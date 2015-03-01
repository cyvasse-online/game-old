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
#include <fea/ui/window.hpp>
#include <cyvmath/match.hpp>

class CyvasseApp : public fea::Application
{
	public:
		CyvasseApp();
		virtual ~CyvasseApp() = default;

	protected:
		void loop() override;

	private:
		fea::Window m_window;
		fea::Renderer2D m_renderer;
		fea::GameStateMachine m_stateMachine;

		std::unique_ptr<cyvmath::Match> m_match;
};

#endif // _CYVASSE_APP_HPP_
