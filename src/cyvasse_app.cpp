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

void CyvasseApp::setup(const std::vector<std::string>& args)
{
	_window.create(fea::VideoMode(800, 600, 32), "Cyvasse");
	_window.setFramerateLimit(60);

	_renderer.setup();
}

void CyvasseApp::loop()
{
	fea::Event event;
	while(_input.pollEvent(event))
	{
		// won't happen when compiled to js,
		// may be of interest somewhen later
		//if(event.type == fea::Event::CLOSED)

		// may be of interest somewhen later
		//if(event.type == fea::Event::KEYPRESSED)

		// TODO: check for mouse events
	}

	// after events were processed
	// * clear the rendered content from the last frame
	_renderer.clear();

	// * render something
	// TODO

	// * display the rendered things
	_window.swapBuffers();
}

void CyvasseApp::destroy()
{
}

CyvasseApp::CyvasseApp()
	: _window(new fea::SDLWindowBackend())
	, _input(new fea::SDLInputBackend())
	, _renderer(fea::Viewport({800.0f, 600.0f}, {0, 0}, fea::Camera({800.0f / 2.0f, 600.0f / 2.0f})))
{
}
