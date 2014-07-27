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

#include "cyvasse_ws_client.hpp"

#include <iostream>
#include <cassert>
#include <json/reader.h>
#include <json/writer.h>
#ifdef EMSCRIPTEN
	#include "websocket_impl_emscripten.inl"
#else
	#include "websocket_impl_native.inl"
#endif

CyvasseWSClient* CyvasseWSClient::_instance = new CyvasseWSClient();

void CyvasseWSClient::handleMessageWrap(const std::string& msg)
{
	if(instance().handleMessage) // if std::function object holds a callable
	{
		try
		{
			Json::Value val;
			if((Json::Reader()).parse(msg, val, false))
				CyvasseWSClient::instance().handleMessage(val);
		}
		catch(std::exception& e)
		{
			std::cerr << "Caught a std::exception while processing a remote message: " << e.what() << '\n';
		}
	}
}

CyvasseWSClient::CyvasseWSClient()
	: wsImpl(new WebsocketImpl())
{ }

CyvasseWSClient::~CyvasseWSClient()
{
	delete wsImpl;
}

CyvasseWSClient& CyvasseWSClient::instance()
{
	assert(_instance);
	return *_instance;
}

void CyvasseWSClient::send(const std::string& str)
{
	wsImpl->send(str);
}

void CyvasseWSClient::send(const Json::Value& val)
{
	send((Json::FastWriter()).write(val));
}
