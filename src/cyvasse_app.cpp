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

#include "cyvasse_app.hpp"

#include <featherkit/ui/sdlwindowbackend.hpp>
#include <featherkit/ui/sdlinputbackend.hpp>
#include "ingame_state.hpp"

void CyvasseApp::setup(const std::vector<std::string>& args)
{
	_window.create(fea::VideoMode(800, 600, 32), "Cyvasse");
	_window.setFramerateLimit(60);

	_renderer.setup();

	_stateMachine.addGameState("ingame", std::unique_ptr<IngameState>(new IngameState(_input, _renderer)));

	// set the initial state
	_stateMachine.setCurrentState("ingame");
}

void CyvasseApp::loop()
{
	// let the state machine run the current game state
	_stateMachine.run();

	// display whatever the current game state rendered
	_window.swapBuffers();

	// exit the program when the state machine is finished, which never happens
	// with the current code because no game state ever returns "NONE"
	if(_stateMachine.isFinished())
		quit();
}

void CyvasseApp::destroy()
{
	_window.close();
}

CyvasseApp::CyvasseApp()
	: _window(new fea::SDLWindowBackend())
	, _input(new fea::SDLInputBackend())
	, _renderer(fea::Viewport({800.0f, 600.0f}, {0, 0}, fea::Camera({800.0f / 2.0f, 600.0f / 2.0f})))
{
}
