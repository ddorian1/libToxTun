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

#include "ToxTun.hpp"
#include "Logger.hpp"
#include "Error.hpp"
#include "Data.hpp"

#include <cstring>

ToxTun::ToxTun(Tox *tox)
:
	tox(tox),
	state(State::Idle),
	callbackFunction(nullptr),
	callbackUserData(nullptr)
{
	tox_callback_friend_lossless_packet(
			tox,
			losslessPacketCallback,
			reinterpret_cast<void *>(this)
	);
}

ToxTun::~ToxTun() {
	try {
		switch (state) {
			case State::RequestPending:
			case State::ExpectingIpPacket:
				resetConnection(connectedFriend);
				break;
			case State::Connected:
				closeConnection();
				break;
			case State::Idle:
				break;
		}
	} catch (Error &error) {
		handleError(error);
	}

	tox_callback_friend_lossless_packet(tox, nullptr, nullptr);
}

ToxTun* ToxTun::newToxTunNoExp(Tox *tox) {
	ToxTun *t = nullptr;

	try {
		t = new ToxTun(tox);
	} catch (Error &error) {
		if (error.get() == Error::Err::Permanent)
			t = nullptr;
	}
	
	return t;
}

void ToxTun::losslessPacketCallback(
		Tox *tox, uint32_t friendNumber,
		const uint8_t *dataRaw,
		size_t length,
		void *ToxTunVoid
) {
	ToxTun *toxTun = reinterpret_cast<ToxTun *>(ToxTunVoid);

	try {
		Data data(Data::fromToxData(dataRaw, length));

		switch(data.getToxHeader()) {
			case Data::PacketId::ConnectionRequest:
				toxTun->handleConnectionRequest(friendNumber);
				break;
			case Data::PacketId::ConnectionAccept:
				toxTun->handleConnectionAccepted(friendNumber);
				break;
			case Data::PacketId::ConnectionReject:
				toxTun->handleConnectionRejected(friendNumber);
				break;
			case Data::PacketId::ConnectionClose:
				toxTun->handleConnectionClosed(friendNumber);
				break;
			case Data::PacketId::ConnectionReset:
				toxTun->handleConnectionReset(friendNumber);
				break;
			case Data::PacketId::IP:
				toxTun->setIp(data, friendNumber);
				break;
			case Data::PacketId::DataFrag:
				toxTun->sendToTun(data, friendNumber, true);
				break;
			case Data::PacketId::DataEnd:
				toxTun->sendToTun(data, friendNumber);
				break;
		}
	} catch (Error &error) {
		toxTun->handleError(error);
	}
}

void ToxTun::setCallback(CallbackFunction callback, void *userData) {
	callbackFunction = callback;
	callbackUserData = userData;
}

void ToxTun::iterate() {
	if (state == State::Connected) {
		try {
			while (tun.dataPending()) 
				sendToTox(tun.getData(), connectedFriend);
		} catch (Error &error) {
			handleError(error);
		}
	}
}

void ToxTun::sendConnectionRequest(uint32_t friendNumber) {
	if (state != State::Idle) {
		Logger::debug("Can't send connection request while not idle");
		return;
	}

	state = State::RequestPending;
	connectedFriend = friendNumber;

	try {
		Data data(Data::fromPacketId(Data::PacketId::ConnectionRequest));
		sendToTox(data, friendNumber);
		Logger::debug("Send connectionRequest to ", friendNumber);
	} catch (Error &error) {
		handleError(error, true);
	}
}

void ToxTun::resetConnection(uint32_t friendNumber) {
	if (friendNumber == connectedFriend && state != State::Idle) {
		//TODO Maybe make this a different event
		callbackFunction(
				Event::ConnectionClosed,
				friendNumber,
				callbackUserData
		);
		if (state == State::Connected) tun.unsetIp();
		state = State::Idle;
	}

	Data data(Data::fromPacketId(Data::PacketId::ConnectionReset));
	sendToTox(data, friendNumber);

	Logger::debug("Reset connection to ", friendNumber);
}

void ToxTun::handleConnectionRequest(uint32_t friendNumber) {
	Logger::debug("ConnectionRequest received from ", friendNumber);

	if (state != State::Idle) {
		if (friendNumber == connectedFriend) {
			Logger::debug("Received connectionRequest from allready connected Friend");
			resetConnection(friendNumber);
		} else {
			Data data(Data::fromPacketId(Data::PacketId::ConnectionReject));
			sendToTox(data, friendNumber);
			Logger::debug("Rejecting incoming connection request");
		}
		return;
	}

	callbackFunction(
			Event::ConnectionRequested,
			friendNumber,
			callbackUserData
	);
}

void ToxTun::handleConnectionAccepted(uint32_t friendNumber) {
	if (state != State::RequestPending || connectedFriend != friendNumber) {
		Logger::debug("Unexpected connectionAccepted received from ", friendNumber);
		resetConnection(friendNumber);
		return;
	}

	//TODO negotiate IP
	Logger::debug("Sending IP postfix ", 2, " to friend ", friendNumber);
	Data data(Data::fromIpPostfix(2));
	sendToTox(data, friendNumber);
	tun.setIp(1);

	state = State::Connected;
	Logger::debug("Connected to ", friendNumber);
	callbackFunction(Event::ConnectionAccepted, friendNumber, callbackUserData);
}
	
void ToxTun::handleConnectionRejected(uint32_t friendNumber) {
	if (state != State::RequestPending || connectedFriend != friendNumber) {
		Logger::debug("Unexpected connectionReject received from ", friendNumber);
		resetConnection(friendNumber);
		return;
	}

	state = State::Idle;
	Logger::debug("Connection rejected from ", friendNumber);

	callbackFunction(Event::ConnectionRejected, friendNumber, callbackUserData);
}

void ToxTun::handleConnectionClosed(uint32_t friendNumber) {
	if (state != State::Connected || connectedFriend != friendNumber) {
		Logger::debug("Received connectionClose from ", friendNumber);
		/* Call the callback in case the friend want's to stop ringing */
		//TODO Maybe make this a different event
		callbackFunction(Event::ConnectionClosed,
				friendNumber,
				callbackUserData
		);
		return;
	}

	callbackFunction(Event::ConnectionClosed, friendNumber, callbackUserData);

	Logger::debug("Closing connection to ", friendNumber);
	tun.unsetIp();
	state = State::Idle;
}

void ToxTun::handleConnectionReset(uint32_t friendNumber) {
	Logger::debug("ConnectionReset received from ", friendNumber);
	if (state != State::Idle && connectedFriend == friendNumber) {
		state = State::Idle;
		//TODO Maybe make this a different event
		callbackFunction(
				Event::ConnectionClosed,
				friendNumber,
				callbackUserData
		);
		Logger::debug("Closing connection");
	}
}

void ToxTun::setIp(const Data &data, uint32_t friendNumber) {
	if (state != State::ExpectingIpPacket || connectedFriend != friendNumber) {
		Logger::debug("Received unexpected IpPacket from ", friendNumber);
		resetConnection(friendNumber);
		return;
	}
	
	uint8_t postfix = data.getIpPostfix();
	tun.setIp(postfix);

	state = State::Connected;
	Logger::debug("Ip set with postfix ", static_cast<int>(postfix));
}

void ToxTun::sendToTun(const Data &data, uint32_t friendNumber, bool fragment) {
	if (fragment) {
		dataFragments.push_back(data);
		Logger::debug("Receving fragmented data packet");
	} else {
		if (!dataFragments.empty()) {
			Logger::debug("Fragmented data packet Received");

			dataFragments.push_back(data);
			Data tmpData = Data::fromToxData(dataFragments);
			tun.sendData(tmpData);
			dataFragments.clear();
		} else
			tun.sendData(data);
	}
}

void ToxTun::sendToTox(const Data &data, uint32_t friendNumber) const {
	std::list<Data> dataList;

	if (data.getToxDataLen() <= TOX_MAX_CUSTOM_PACKET_SIZE) {
		dataList.push_back(data);
	} else {
		Logger::debug("Packet to big for tox, splitting it");

		size_t len = data.getIpDataLen();
		uint8_t buf[TOX_MAX_CUSTOM_PACKET_SIZE];
		size_t pos = 0;

		while (pos < len) {
			size_t toCpy = (len - pos < TOX_MAX_CUSTOM_PACKET_SIZE - 1) ?
				len - pos : TOX_MAX_CUSTOM_PACKET_SIZE - 1;
			memcpy(buf, &(data.getIpData()[pos]), toCpy);
			pos += toCpy;

			Data tmp = Data::fromTunData(buf, toCpy, pos != len);
			dataList.push_back(tmp);
		}
	}

	for (const auto &d : dataList) {
		bool ok = tox_friend_send_lossless_packet(
				tox,
				friendNumber,
				d.getToxData(),
				d.getToxDataLen(),
				NULL
		);

		if (!ok) {
			Logger::error("Can't send lossless packet to ", friendNumber);
			throw Error(Error::Err::Temp);
		}
	}
}

void ToxTun::acceptConnection(uint32_t friendNumber) {
	if (state != State::Idle) {
		Logger::debug("Can't accept connection while not idle");
		return;
	}

	state = State::ExpectingIpPacket;
	connectedFriend = friendNumber;

	try {
		Data data(Data::fromPacketId(Data::PacketId::ConnectionAccept));
		sendToTox(data, friendNumber);
	} catch (Error &error) {
		handleError(error, true);
	}

	Logger::debug("Accepting connection from ", friendNumber);
}

void ToxTun::rejectConnection(uint32_t friendNumber) {
	try {
		Data data(Data::fromPacketId(Data::PacketId::ConnectionReject));
		sendToTox(data, friendNumber);
	} catch (Error &error) {
		handleError(error, true);
	}

	Logger::debug("Rejecting connection from ", friendNumber);
}

void ToxTun::closeConnection() {
	if (state == State::Idle) {
		Logger::debug("No connection to close");
		return;
	}

	try {
		Data data(Data::fromPacketId(Data::PacketId::ConnectionClose));
		sendToTox(data, connectedFriend);
		tun.unsetIp();
	} catch (Error &error) {
		handleError(error, true);
	}

	state = State::Idle;
	Logger::debug("Closing connection to ", connectedFriend);
}

void ToxTun::handleError(Error &error, bool sensitiv) {
	try {
		switch (error.get()) {
			case Error::Err::Temp:
				if (sensitiv)
					resetConnection(connectedFriend);
				break;
			case Error::Err::Critical:
				resetConnection(connectedFriend);
				break;
			case Error::Err::Permanent:
				state = State::PermanentError;
				break;
		}
	} catch (Error &error) {};
}
