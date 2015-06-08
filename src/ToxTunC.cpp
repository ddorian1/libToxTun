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

static void (*cCallback)(
		enum CCallbackEvents event,
		uint32_t friendNumber,
		void *userData
) = nullptr;

static void intermediateCallback(
		ToxTun::Event event, 
		uint32_t friendNumber,
		void *userData
) {
	if (!cCallback) return;

	enum CCallbackEvents cEvent;
	
	switch(event) {
		case ToxTun::Event::ConnectionRequested:
			cEvent = connection_requested;
			break;
		case ToxTun::Event::ConnectionAccepted:
			cEvent = connection_accepted;
			break;
		case ToxTun::Event::ConnectionRejected:
			cEvent = connection_rejected;
			break;
		case ToxTun::Event::ConnectionClosed:
			cEvent = connection_closed;
			break;
	}

	cCallback(cEvent, friendNumber, userData);
}

void *toxtun_new(Tox *tox) {
	ToxTun *t;
	try {
		t = new ToxTun(tox);
	} catch (...) {
		return nullptr;
	}
	return reinterpret_cast<void *>(t);
}

void toxtun_kill(void *toxtun) {
	ToxTun *t = reinterpret_cast<ToxTun *>(toxtun);
	try {
		t->closeConnection();
		delete t;
	} catch (...) {}
}

void toxtun_set_callback(
		void *toxtun,
		void (*cb)(
			enum CCallbackEvents event,
			uint32_t friendNumber,
			void *userData
		),
		void *userData
) {
	ToxTun *t = reinterpret_cast<ToxTun *>(toxtun);
	cCallback = cb;
	t->setCallback(intermediateCallback, userData);
}
	
bool toxtun_iterate(void *toxtun) {
	ToxTun *t = reinterpret_cast<ToxTun *>(toxtun);
	try {
		t->iterate();
	} catch (...) {
		return false;
	}
	return true;
}


bool toxtun_send_connection_request(void *toxtun, uint32_t friendNumber) {
	ToxTun *t = reinterpret_cast<ToxTun *>(toxtun);
	try {
		t->sendConnectionRequest(friendNumber);
	} catch (...) {
		return false;
	}
	return true;
}

bool toxtun_accept_connection(void *toxtun, uint32_t friendNumber) {
	ToxTun *t = reinterpret_cast<ToxTun *>(toxtun);
	try {
		t->acceptConnection(friendNumber);
	} catch (...) {
		return false;
	}
	return true;
}

bool toxtun_reject_connection(void *toxtun, uint32_t friendNumber) {
	ToxTun *t = reinterpret_cast<ToxTun *>(toxtun);
	try {
		t->rejectConnection(friendNumber);
	} catch (...) {
		return false;
	}
	return true;
}

bool toxtun_close_connection(void *toxtun) {
	ToxTun *t = reinterpret_cast<ToxTun *>(toxtun);
	try {
		t->closeConnection();
	} catch (...) {
		return false;
	}
	return true;
}
