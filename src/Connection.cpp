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

#include "Connection.hpp"
#include "ToxTunCore.hpp"
#include "Logger.hpp"
#include "Data.hpp"

#include <cstring>

Connection::Connection(uint32_t friendNumber, ToxTunCore &toxTunCore, bool initiateConnection)
:
	toxTunCore(toxTunCore),
	tun(toxTunCore.getTox()),
	state(
			initiateConnection ? 
			State::OwnRequestPending : State::FriendsRequestPending
	),
	connectedFriend(friendNumber)
{
	if (initiateConnection)
		sendConnectionRequest();
}

Connection::~Connection() {
	switch (state) {
		case State::FriendsRequestPending:
			rejectConnection();
			break;
		case State::OwnRequestPending:
		case State::ExpectingIpPacket:
			resetConnection();
			break;
		case State::Connected:
			closeConnection();
			break;
		case State::Deleting:
			break;
	}
}

void Connection::rejectConnection() noexcept {
	Data data(Data::fromPacketId(Data::PacketId::ConnectionReject));
	try {
		sendToTox(data);
	} catch (ToxTunError &error) {}
}

void Connection::closeConnection() noexcept {
	Data data(Data::fromPacketId(Data::PacketId::ConnectionClose));
	try {
		sendToTox(data);
	} catch (ToxTunError &error) {}
}

void Connection::handleData(const Data &data) noexcept {
	switch(data.getToxHeader()) {
		case Data::PacketId::ConnectionRequest:
			//This should never be reached
			Logger::error("Connection doesn't handle connection requestes");
			break;
		case Data::PacketId::ConnectionAccept:
			handleConnectionAccepted();
			break;
		case Data::PacketId::ConnectionReject:
			handleConnectionRejected();
			break;
		case Data::PacketId::ConnectionClose:
			handleConnectionClosed();
			break;
		case Data::PacketId::ConnectionReset:
			handleConnectionReset();
			break;
		case Data::PacketId::IP:
			setIp(data);
			break;
		case Data::PacketId::Data:
			sendToTun(data);
			break;
		case Data::PacketId::Fragment:
			handleFragment(data);
			break;
	}
}

void Connection::iterate(std::chrono::duration<double> time) noexcept {
	const auto tStart = std::chrono::high_resolution_clock::now();
	while(true) {
		if (state != State::Connected || !tun.dataPending()) break;

		const auto tNow = std::chrono::high_resolution_clock::now();
		const auto tElapsed = std::chrono::duration_cast<std::chrono::duration<double>>(
				tNow - tStart
		);
		if (tElapsed > time) break;

		try {
			sendToTox(tun.getData());
		} catch (ToxTunError &error) {
			//TODO
			//handleError(error);
		}
	}
}

void Connection::sendConnectionRequest() {
	Data data(Data::fromPacketId(Data::PacketId::ConnectionRequest));
	sendToTox(data);
	Logger::debug("Send connectionRequest to ", connectedFriend);
}

void Connection::resetConnection() noexcept {
	resetConnection(connectedFriend, toxTunCore.getTox());
	deleteConnection();

	toxTunCore.callback(
			ToxTun::Event::ConnectionClosed,
			connectedFriend
	);
}

void Connection::resetConnection(uint32_t friendNumber, Tox *tox) noexcept {
	Data data(Data::fromPacketId(Data::PacketId::ConnectionReset));
	try {
		sendToTox(data, friendNumber, tox);
	} catch (ToxTunError &error) {}

	Logger::debug("Reset connection to ", friendNumber);
}

void Connection::handleConnectionAccepted() noexcept {
	if (state != State::OwnRequestPending) {
		Logger::debug("Unexpected connectionAccepted received from ", connectedFriend);
		resetConnection();
		return;
	}

	//TODO negotiate IP
	Logger::debug("Sending IP postfix ", 2, " to friend ", connectedFriend);
	Data data(Data::fromIpPostfix(2));
	try {
		sendToTox(data);
	} catch (ToxTunError &error) {
		resetConnection();
		return;
	}

	tun.setIp(1); //TODO make shure this doesn't throw
	state = State::Connected;
	Logger::debug("Connected to ", connectedFriend);
	toxTunCore.callback(
			ToxTun::Event::ConnectionAccepted,
			connectedFriend
	);
}
	
void Connection::handleConnectionRejected() noexcept {
	if (state != State::OwnRequestPending) {
		Logger::debug("Unexpected connectionReject received from ", connectedFriend);
		resetConnection();
		return;
	}

	Logger::debug("Connection rejected from ", connectedFriend);

	toxTunCore.callback(
			ToxTun::Event::ConnectionRejected,
			connectedFriend
	);
	deleteConnection();
}

void Connection::handleConnectionClosed() noexcept {
	if (state != State::Connected) {
		Logger::debug("Received connectionClose from ", connectedFriend);
		resetConnection();
		return;
	}

	toxTunCore.callback(
			ToxTun::Event::ConnectionClosed, 
			connectedFriend
	);

	Logger::debug("Closing connection to ", connectedFriend);
	deleteConnection();
}

void Connection::handleConnectionReset() noexcept {
	Logger::debug("ConnectionReset received from ", connectedFriend);
	deleteConnection();
}

void Connection::setIp(const Data &data) noexcept {
	if (state != State::ExpectingIpPacket) {
		Logger::debug("Received unexpected IpPacket from ", connectedFriend);
		resetConnection();
		return;
	}

	uint8_t postfix;
	try {
		postfix = data.getIpPostfix();
		tun.setIp(postfix);
		Logger::debug("Ip set with postfix ", static_cast<int>(postfix));
	} catch (ToxTunError &error) {
		Logger::error("Can't set Ip with postfix ", static_cast<int>(postfix));
		return;
	}

	state = State::Connected;
}

void Connection::sendToTun(const Data &data) noexcept {
	if (state != State::Connected) {
		Logger::error("Received data package from not connected friend");
		resetConnection();
	}
	try {
		tun.sendData(data);
	} catch (ToxTunError &error) {};
}

void Connection::sendToTox(const Data &data) const {
	sendToTox(data, connectedFriend, toxTunCore.getTox());
}

void Connection::sendToTox(const Data &data, uint32_t friendNumber, Tox *tox){
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
					throw ToxTunError(Logger::concat("Can't send lossless packet to ", friendNumber));
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
					throw ToxTunError(Logger::concat("Can't send lossy packet to ", friendNumber));
				}
				break;
		}
	}
}

void Connection::acceptConnection() {
	if (state != State::FriendsRequestPending) {
		throw ToxTunError("Connection not on right state to accept connection request");
		return;
	}

	state = State::ExpectingIpPacket;

	Data data(Data::fromPacketId(Data::PacketId::ConnectionAccept));
	sendToTox(data);

	Logger::debug("Accepting connection from ", connectedFriend);
}

void Connection::deleteConnection() noexcept {
	state = State::Deleting;
	toxTunCore.deleteConnection(connectedFriend);
}

void Connection::handleFragment(const Data &data) noexcept {
	if (!data.isValidFragment()) return;

	const uint8_t sdi = data.getSplittedDataIndex();

	if (!fragments.count(sdi)) {
		std::list<Data> l = {data};
		fragments.emplace(sdi, std::move(l));
	} else {
		fragments.at(sdi).push_front(data);
	}

	if (fragments.at(sdi).size() == data.getFragmentsCount()) {
		std::list<Data> tmp(std::move(fragments.at(sdi)));
		fragments.erase(sdi);

		for (size_t i=sdi+128u;i<sdi+128u+3u;++i)
			fragments.erase(i%256);

		handleData(Data::fromFragments(tmp));

		Logger::debug("fragments[", connectedFriend, "].size() == ", fragments.size());
	}
}
