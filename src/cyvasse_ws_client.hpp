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

#ifndef _CYVASSE_WS_CLIENT_HPP_
#define _CYVASSE_WS_CLIENT_HPP_

#include <functional>
#include <json/value.h>

class WebsocketImpl;

class CyvasseWSClient
{
	private:
		WebsocketImpl* wsImpl;

		// implemented as singleton because one game can only
		// be connected to one websocket remote end at once
		static CyvasseWSClient* _instance;

	public:
		CyvasseWSClient();
		~CyvasseWSClient();

		std::function<void(const Json::Value&)> handleMessage;

		static CyvasseWSClient& instance();

		void send(const std::string&);
		void send(const Json::Value&);
};

#endif // _CYVASSE_WS_CLIENT_HPP_
