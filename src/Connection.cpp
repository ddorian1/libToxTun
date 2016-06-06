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
	connectedFriend(friendNumber),
	nextFragmentIndex(0),
	subnet(-1)
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
		case State::ExpectingIpConfirmation:
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
		case Data::PacketId::IpProposal:
			handleIpProposal(data);
			break;
		case Data::PacketId::IpAccept:
			handleIpAccepted();
			break;
		case Data::PacketId::IpReject:
			handleIpRejected();
			break;
		case Data::PacketId::Data:
			sendToTun(data);
			break;
		case Data::PacketId::Fragment:
			handleFragment(data);
			break;
	}
}

void Connection::iterate(std::chrono::milliseconds time) noexcept {
	const auto timeStart = std::chrono::steady_clock::now();
	while(true) {
		if (state != State::Connected || !tun.dataPending()) break;

		const auto timeElapsed = std::chrono::steady_clock::now() - timeStart;
		if (timeElapsed > time) break;

		try {
			sendToTox(tun.getData());
		} catch (ToxTunError &error) {}
	}
}

void Connection::sendConnectionRequest() {
	Data data(Data::fromPacketId(Data::PacketId::ConnectionRequest));
	sendToTox(data);
	Logger::debug("Send connectionRequest to ", connectedFriend);
}

void Connection::resetConnection() noexcept {
	resetConnection(connectedFriend, toxTunCore.getTox());

	toxTunCore.callback(
			ToxTun::Event::ConnectionClosed,
			connectedFriend
	);
}

void Connection::resetAndDeleteConnection() noexcept {
	resetConnection();
	deleteConnection();
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
		resetAndDeleteConnection();
		return;
	}

	Logger::debug("Start to negotiate Ip with friend ", connectedFriend);
	state = State::ExpectingIpConfirmation;
	sendIp();
}

void Connection::handleConnectionRejected() noexcept {
	if (state != State::OwnRequestPending) {
		Logger::debug("Unexpected connectionReject received from ", connectedFriend);
		resetAndDeleteConnection();
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
		resetAndDeleteConnection();
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

	toxTunCore.callback(
			ToxTun::Event::ConnectionClosed,
			connectedFriend
	);
	deleteConnection();
}

void Connection::handleIpProposal(const Data &data) noexcept {
	if (state != State::ExpectingIpPacket) {
		Logger::debug("Received unexpected IpProposal from ", connectedFriend);
		resetAndDeleteConnection();
		return;
	}

	uint8_t postfix, subnet;
	try {
		postfix = data.getIpPostfix();
		subnet = data.getIpSubnet();
	} catch (ToxTunError &error) {
		Logger::error("Received invalid IpProposal from ", connectedFriend);
		return;
	}

	bool unused;
	try {
		unused = tun.isAddrspaceUnused(subnet);
	} catch (ToxTunError &error) {
		Logger::error("Can't check if subnet is used, assuming it is not");
		unused = true;
	}

	if (unused) {
		Logger::debug("Address space ", static_cast<int>(subnet), " unused");
		Data data = Data::fromPacketId(Data::PacketId::IpAccept);
		try {
			sendToTox(data);
		} catch (ToxTunError &error) {
			resetAndDeleteConnection();
		}
		setIp(subnet, postfix);
	} else {
		Logger::debug("Address space ", static_cast<int>(subnet), " used");
		Data data = Data::fromPacketId(Data::PacketId::IpReject);
		try {
			sendToTox(data);
		} catch (ToxTunError &error) {
			resetAndDeleteConnection();
		}
	}
}

void Connection::setIp(uint8_t subnet, uint8_t postfix) noexcept {
	try {
		tun.setIp(subnet, postfix);
		Logger::debug("Ip set to 192.168.",
				static_cast<int>(subnet), ".",
				static_cast<int>(postfix)
		);
	} catch (ToxTunError &error) {
		Logger::error(
				"Can't set Ip to 192.168.",
				static_cast<int>(subnet), ".",
				static_cast<int>(postfix)
		);
		return;
	}

	state = State::Connected;
	toxTunCore.callback(
			ToxTun::Event::ConnectionAccepted,
			connectedFriend
	);
}

void Connection::handleIpAccepted() noexcept {
	if (state != State::ExpectingIpConfirmation) {
		Logger::debug("Received unexpected IpAccept from ", connectedFriend);
		resetAndDeleteConnection();
		return;
	}

	setIp(subnet, 1);
}

void Connection::handleIpRejected() noexcept {
	if (state != State::ExpectingIpConfirmation) {
		Logger::debug("Received unexpected IpReject from ", connectedFriend);
		resetAndDeleteConnection();
		return;
	}

	sendIp();
}

void Connection::sendIp() noexcept {
	bool unused = false;
	while (!unused) {
		++subnet;

		try {
			unused = tun.isAddrspaceUnused(subnet);
		} catch (ToxTunError &error) {
			Logger::error("Can't check if subnet is used, assuming it is not");
			unused = true;
		}

		if (subnet == 256) {
			Logger::error("No free Ip subnet avaible");
			resetAndDeleteConnection();
			return;
		}
	}

	Data data = Data::fromIpPostfix(subnet, 2);
	try {
		sendToTox(data);
	} catch (ToxTunError &error) {
		resetAndDeleteConnection();
	}
}

void Connection::sendToTun(const Data &data) noexcept {
	if (state != State::Connected) {
		Logger::error("Received data package from not connected friend");
		resetAndDeleteConnection();
	}
	try {
		tun.sendData(data);
	} catch (ToxTunError &error) {};
}

void Connection::sendToTox(const Data &data) {
	sendToTox(data, connectedFriend, toxTunCore.getTox(), &nextFragmentIndex);
}

void Connection::sendToTox(const Data &data, uint32_t friendNumber, Tox *tox, uint8_t *nextFragmentIndex){
	std::forward_list<Data> dataList;

	if (data.getToxDataLen() <= TOX_MAX_CUSTOM_PACKET_SIZE) {
		dataList.push_front(data);
	} else {
		Logger::debug("Packet to big for tox, splitting it");
		uint8_t index = 0;
		if (nextFragmentIndex) {
			index = *nextFragmentIndex;
			*nextFragmentIndex = (*nextFragmentIndex == 255) ?
				0 : *nextFragmentIndex + 1;
		}
		dataList = std::move(data.getSplitted(index));
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
	}

	state = State::ExpectingIpPacket;

	Data data(Data::fromPacketId(Data::PacketId::ConnectionAccept));
	try {
		sendToTox(data);
	} catch (ToxTunError &error) {
		resetAndDeleteConnection();
		return;
	}

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

ToxTun::ConnectionState Connection::getConnectionState() noexcept {
	switch (state) {
		case State::Connected:
			return ToxTun::ConnectionState::Connected;
			break;
		case State::Deleting:
			return ToxTun::ConnectionState::Disconnected;
			break;
		case State::FriendsRequestPending:
		case State::ExpectingIpPacket:
			return ToxTun::ConnectionState::FriendIsRinging;
			break;
		case State::OwnRequestPending:
		case State::ExpectingIpConfirmation:
			return ToxTun::ConnectionState::RingingAtFriend;
			break;
		default:
			return ToxTun::ConnectionState::Disconnected;
			break;
	}
}
