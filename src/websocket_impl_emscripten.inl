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

#include <functional>
#include <emscripten.h>
#include <json/reader.h>

class WebsocketImpl
{
	public:
		void send(const std::string& msgData);
};

void WebsocketImpl::send(const std::string& msgData)
{
	EM_ASM_({
		var jsWSClient = Module.wsClient;

		if(jsWSClient.debug === true) {
	        console.log('[send]');
	        console.log($0);
	    }

		jsWSClient.conn.send($0);
	}, msgData.c_str());
}

extern "C"
{
	void game_handlemessage(const char* msgData)
	{
		if(CyvasseWSClient::instance().handleMessage) // if std::function object holds a callable
		{
			Json::Value val;
			if((Json::Reader()).parse(std::string(msgData), val, false))
				CyvasseWSClient::instance().handleMessage(val);
		}
	}
}
