/* 
 * Copyright (C) 2015 Johannes Schwab
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ToxTunC.h"
#include "ToxTun.hpp"

#include <map>
#include <memory>
#include <cstring>

static std::map<void*, std::string> errorStrings;

static void (*cCallback)(
		enum toxtun_event event,
		uint32_t friendNumber,
		void *userData
) = nullptr;

static void intermediateCallback(
		ToxTun::Event event, 
		uint32_t friendNumber,
		void *userData
) {
	if (!cCallback) return;

	enum toxtun_event cEvent;
	
	switch(event) {
		case ToxTun::Event::ConnectionRequested:
			cEvent = TOXTUN_CONNECTION_REQUESTED;
			break;
		case ToxTun::Event::ConnectionAccepted:
			cEvent = TOXTUN_CONNECTION_ACCEPTED;
			break;
		case ToxTun::Event::ConnectionRejected:
			cEvent = TOXTUN_CONNECTION_REJECTED;
			break;
		case ToxTun::Event::ConnectionClosed:
			cEvent = TOXTUN_CONNECTION_CLOSED;
			break;
	}

	cCallback(cEvent, friendNumber, userData);
}

void *toxtun_new(Tox *tox) {
	ToxTun *t = ToxTun::newToxTunNoExp(tox);
	return reinterpret_cast<void *>(t);
}

void toxtun_kill(void *toxtun) {
	ToxTun *t = reinterpret_cast<ToxTun *>(toxtun);
	delete t;
}

void toxtun_set_callback(
		void *toxtun,
		void (*cb)(
			enum toxtun_event event,
			uint32_t friendNumber,
			void *userData
		),
		void *userData
) {
	ToxTun *t = reinterpret_cast<ToxTun *>(toxtun);
	cCallback = cb;
	t->setCallback(intermediateCallback, userData);
}
	
void toxtun_iterate(void *toxtun) {
	ToxTun *t = reinterpret_cast<ToxTun *>(toxtun);
	t->iterate();
}


bool toxtun_send_connection_request(void *toxtun, uint32_t friendNumber) {
	ToxTun *t = reinterpret_cast<ToxTun *>(toxtun);
	try {
		t->sendConnectionRequest(friendNumber);
	} catch (ToxTunError &error) {
		errorStrings[toxtun] = error.what();
		return false;
	}

	return true;
}

bool toxtun_accept_connection(void *toxtun, uint32_t friendNumber) {
	ToxTun *t = reinterpret_cast<ToxTun *>(toxtun);
	try {
		t->acceptConnection(friendNumber);
	} catch (ToxTunError &error) {
		errorStrings[toxtun] = error.what();
		return false;
	}

	return true;
}

void toxtun_reject_connection(void *toxtun, uint32_t friendNumber) {
	ToxTun *t = reinterpret_cast<ToxTun *>(toxtun);
	t->rejectConnection(friendNumber);
}

void toxtun_close_connection(void *toxtun, uint32_t friendNumber) {
	ToxTun *t = reinterpret_cast<ToxTun *>(toxtun);
	t->closeConnection(friendNumber);
}

enum toxtun_connection_state toxtun_get_connection_state(void *toxtun, uint32_t friendNumber) {
	ToxTun *t = reinterpret_cast<ToxTun *>(toxtun);
	switch (t->getConnectionState(friendNumber)) {
		case ToxTun::ConnectionState::Connected:
			return TOXTUN_CONNECTION_STATE_CONNECTED;
			break;
		case ToxTun::ConnectionState::Disconnected:
			return TOXTUN_CONNECTION_STATE_DISCONNECTED;
			break;
		case ToxTun::ConnectionState::RingingAtFriend:
			return TOXTUN_CONNECTION_STATE_RINGING_AT_FRIEND;
			break;
		case ToxTun::ConnectionState::FriendIsRinging:
			return TOXTUN_CONNECTION_STATE_FRIEND_IS_RINGING;
			break;
		default:
			return TOXTUN_CONNECTION_STATE_DISCONNECTED;
	}
}

const char* toxtun_get_last_error(void *toxtun) {
	static std::map<void*, std::unique_ptr<const char[]>> errorCStrings;

	char *cString;
	if (errorStrings.count(toxtun)) {
		const size_t len = errorStrings.at(toxtun).size();

		cString = new char[len + 1];
		std::strncpy(cString, errorStrings.at(toxtun).c_str(), len + 1);
		cString[len] = '\0';

		errorStrings.erase(toxtun);
	} else {
		cString = new char[9];
		std::strncpy(cString, "No error", 9);
		cString[8] = '\0';
	}

	if (errorCStrings.count(toxtun)) {
		errorCStrings.at(toxtun).reset(cString);
	} else {
		errorCStrings.emplace(
				std::piecewise_construct,
				std::forward_as_tuple(toxtun),
				std::forward_as_tuple(cString)
		);
	}

	return errorCStrings.at(toxtun).get();
}
