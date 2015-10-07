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

#include "ToxTunCore.hpp"
#include "Logger.hpp"
#include "Error.hpp"
#include "Data.hpp"

#include <cstring>

ToxTunCore::ToxTunCore(Tox *tox)
:
	tox(tox),
	state(State::Idle),
	callbackUserData(nullptr),
	callbackFunction(nullptr)
{
	tox_callback_friend_lossless_packet(
			tox,
			toxPacketCallback,
			reinterpret_cast<void *>(this)
	);

	tox_callback_friend_lossy_packet(
			tox,
			toxPacketCallback,
			reinterpret_cast<void *>(this)
	);
}

ToxTunCore::~ToxTunCore() {
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
			case State::PermanentError:
				break;
		}
	} catch (Error &error) {
		handleError(error);
	}

	tox_callback_friend_lossless_packet(tox, nullptr, nullptr);
}

void ToxTunCore::toxPacketCallback(
		Tox *tox, uint32_t friendNumber,
		const uint8_t *dataRaw,
		size_t length,
		void *ToxTunCoreVoid
) {
	ToxTunCore *toxTun = reinterpret_cast<ToxTunCore *>(ToxTunCoreVoid);

	try {
		Data data(Data::fromToxData(dataRaw, length));
		toxTun->handleData(data, friendNumber);
	} catch (Error &error) {
		toxTun->handleError(error);
	}
}

void ToxTunCore::handleData(const Data &data, uint32_t friendNumber) {
	switch(data.getToxHeader()) {
		case Data::PacketId::ConnectionRequest:
			handleConnectionRequest(friendNumber);
			break;
		case Data::PacketId::ConnectionAccept:
			handleConnectionAccepted(friendNumber);
			break;
		case Data::PacketId::ConnectionReject:
			handleConnectionRejected(friendNumber);
			break;
		case Data::PacketId::ConnectionClose:
			handleConnectionClosed(friendNumber);
			break;
		case Data::PacketId::ConnectionReset:
			handleConnectionReset(friendNumber);
			break;
		case Data::PacketId::IP:
			setIp(data, friendNumber);
			break;
		case Data::PacketId::Data:
			sendToTun(data, friendNumber);
			break;
		case Data::PacketId::Fragment:
			handleFragment(data, friendNumber);
			break;
	}
}

void ToxTunCore::setCallback(ToxTun::CallbackFunction callback, void *userData) {
	callbackFunction = callback;
	callbackUserData = userData;
}

void ToxTunCore::iterate() {
	if (state == State::Connected) {
		try {
			/* 
			 * TODO: is 10 a reasonable value?
			 * Or would a timeout be a better solution?
			 */
			for (int i=0;i<10;++i) {
				if (!tun.dataPending()) break;
				sendToTox(tun.getData(), connectedFriend);
			}
		} catch (Error &error) {
			handleError(error);
		}
	}
}

void ToxTunCore::sendConnectionRequest(uint32_t friendNumber) {
	if (state != State::Idle) {
		Logger::debug("Can't send connection request while not idle");
		callbackFunction(
				ToxTun::Event::ConnectionClosed,
				friendNumber,
				callbackUserData
		);
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

void ToxTunCore::resetConnection(uint32_t friendNumber) {
	if (friendNumber == connectedFriend && state != State::Idle) {
		//TODO Maybe make this a different event
		callbackFunction(
				ToxTun::Event::ConnectionClosed,
				friendNumber,
				callbackUserData
		);
		if (state == State::Connected) tun.unsetIp();
		state = State::Idle;
		fragments.clear();
	}

	Data data(Data::fromPacketId(Data::PacketId::ConnectionReset));
	sendToTox(data, friendNumber);

	Logger::debug("Reset connection to ", friendNumber);
}

void ToxTunCore::handleConnectionRequest(uint32_t friendNumber) {
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
			ToxTun::Event::ConnectionRequested,
			friendNumber,
			callbackUserData
	);
}

void ToxTunCore::handleConnectionAccepted(uint32_t friendNumber) {
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
	callbackFunction(ToxTun::Event::ConnectionAccepted, friendNumber, callbackUserData);
}
	
void ToxTunCore::handleConnectionRejected(uint32_t friendNumber) {
	if (state != State::RequestPending || connectedFriend != friendNumber) {
		Logger::debug("Unexpected connectionReject received from ", friendNumber);
		resetConnection(friendNumber);
		return;
	}

	state = State::Idle;
	Logger::debug("Connection rejected from ", friendNumber);

	callbackFunction(ToxTun::Event::ConnectionRejected, friendNumber, callbackUserData);
}

void ToxTunCore::handleConnectionClosed(uint32_t friendNumber) {
	if (state != State::Connected || connectedFriend != friendNumber) {
		Logger::debug("Received connectionClose from ", friendNumber);
		/* Call the callback in case the friend want's to stop ringing */
		//TODO Maybe make this a different event
		callbackFunction(ToxTun::Event::ConnectionClosed,
				friendNumber,
				callbackUserData
		);
		return;
	}

	callbackFunction(ToxTun::Event::ConnectionClosed, friendNumber, callbackUserData);

	Logger::debug("Closing connection to ", friendNumber);
	tun.unsetIp();
	fragments.clear();
	state = State::Idle;
}

void ToxTunCore::handleConnectionReset(uint32_t friendNumber) {
	Logger::debug("ConnectionReset received from ", friendNumber);
	if (state != State::Idle && connectedFriend == friendNumber) {
		state = State::Idle;
		fragments.clear();
		//TODO Maybe make this a different event
		callbackFunction(
				ToxTun::Event::ConnectionClosed,
				friendNumber,
				callbackUserData
		);
		Logger::debug("Closing connection");
	}
}

void ToxTunCore::setIp(const Data &data, uint32_t friendNumber) {
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

void ToxTunCore::sendToTun(const Data &data, uint32_t friendNumber) {
	if (state != State::Connected || friendNumber != connectedFriend) {
		Logger::error("Received data package from not connected friend");
		resetConnection(friendNumber);
		throw (Error(Error::Err::Temp));
	}
	tun.sendData(data);
}

void ToxTunCore::sendToTox(const Data &data, uint32_t friendNumber) const {
	std::forward_list<Data> dataList;

	if (data.getToxDataLen() <= TOX_MAX_CUSTOM_PACKET_SIZE) {
		dataList.push_front(data);
	} else {
		Logger::debug("Packet to big for tox, splitting it");
		dataList = std::move(data.getSplitted());
	}

	bool status;
	for (const auto &d : dataList) {
		switch (d.getSendTox()) {
			case Data::SendTox::Lossless:
				Logger::debug("Sending lossless packet to ", friendNumber);
				status = tox_friend_send_lossless_packet(
						tox,
						friendNumber,
						d.getToxData(),
						d.getToxDataLen(),
						NULL
				);

				if (!status) {
					Logger::error("Can't send lossless packet to ", friendNumber);
					throw Error(Error::Err::Temp);
				}
				break;
			case Data::SendTox::Lossy:
				Logger::debug("Sending lossy packet to ", friendNumber);
				status = tox_friend_send_lossy_packet(
						tox,
						friendNumber,
						d.getToxData(),
						d.getToxDataLen(),
						NULL
				);

				if (!status) {
					Logger::error("Can't send lossy packet to ", friendNumber);
					throw Error(Error::Err::Temp);
				}
				break;
		}
	}
}

void ToxTunCore::acceptConnection(uint32_t friendNumber) {
	if (state != State::Idle) {
		Logger::debug("Can't accept connection while not idle");
		callbackFunction(
				ToxTun::Event::ConnectionClosed,
				friendNumber,
				callbackUserData
		);
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

void ToxTunCore::rejectConnection(uint32_t friendNumber) {
	try {
		Data data(Data::fromPacketId(Data::PacketId::ConnectionReject));
		sendToTox(data, friendNumber);
	} catch (Error &error) {
		handleError(error, true);
	}

	Logger::debug("Rejecting connection from ", friendNumber);
}

void ToxTunCore::closeConnection() {
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
	fragments.clear();
	Logger::debug("Closing connection to ", connectedFriend);
}

void ToxTunCore::handleFragment(const Data &data, uint32_t friendNumber) {
	const uint8_t sdi = data.getSplittedDataIndex();

	if (!fragments.count(friendNumber)) {
		std::list<Data> l = {data};
		std::pair<uint8_t, std::list<Data> > p(sdi, std::move(l));
		std::map<uint8_t, std::list<Data> > m = {std::move(p)};
		fragments.emplace(std::make_pair(friendNumber, std::move(m)));
	} else if (!fragments.at(friendNumber).count(sdi)) {
		std::list<Data> l = {data};
		fragments.at(friendNumber).emplace(std::make_pair(sdi, std::move(l)));
	} else {
		fragments.at(friendNumber).at(sdi).push_front(data);
	}

	if (fragments.at(friendNumber).at(sdi).size() == data.getFragmentsCount()) {
		std::list<Data> tmp(std::move(fragments.at(friendNumber).at(sdi)));
		fragments.at(friendNumber).erase(sdi);

		for (size_t i=sdi+128u;i<sdi+128u+3u;++i)
			fragments.at(friendNumber).erase(i%256);

		handleData(Data::fromFragments(tmp), friendNumber);
		Logger::debug("fragments[", friendNumber, "].size() == ", fragments.at(friendNumber).size());
	}
}

void ToxTunCore::handleError(Error &error, bool sensitiv) {
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
