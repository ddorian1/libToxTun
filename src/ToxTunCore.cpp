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
#include "Connection.hpp"
#include "Logger.hpp"
#include "Data.hpp"

ToxTunCore::ToxTunCore(Tox *tox) noexcept
:
	tox(tox),
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
	tox_callback_friend_lossless_packet(tox, nullptr, nullptr);
	tox_callback_friend_lossy_packet(tox, nullptr, nullptr);
}

void ToxTunCore::toxPacketCallback(
		Tox *tox, uint32_t friendNumber,
		const uint8_t *dataRaw,
		size_t length,
		void *ToxTunCoreVoid
) noexcept {
	ToxTunCore *toxTun = reinterpret_cast<ToxTunCore *>(ToxTunCoreVoid);

	Data data(Data::fromToxData(dataRaw, length));
	toxTun->handleData(data, friendNumber);
}

void ToxTunCore::handleData(const Data &data, uint32_t friendNumber) noexcept {
	if (connections.count(friendNumber)) {
		connections.at(friendNumber).handleData(data);
		return;
	}

	switch (data.getToxHeader()) {
		case Data::PacketId::ConnectionRequest:
			handleConnectionRequest(friendNumber);
			break;
		case Data::PacketId::ConnectionReset:
			Logger::debug("Received ConnectionReset from not connected friend, ignoring");
			break;
		default:
			Logger::debug("Received packet from not connected friend");
			Connection::resetConnection(friendNumber, tox);
	}
}

void ToxTunCore::setCallback(ToxTun::CallbackFunction callback, void *userData) noexcept {
	callbackFunction = callback;
	callbackUserData = userData;
}

void ToxTunCore::iterate() noexcept {
	if (connections.empty()) return;

	const auto timeBegin = std::chrono::steady_clock::now();

	const unsigned int intPerCon =
		tox_iteration_interval(tox) / connections.size();
	const std::chrono::milliseconds timePerConnection(
			intPerCon >= 1 ? intPerCon : 1
	);

	for (auto &connection : connections)
		connection.second.iterate(timePerConnection);

	iterationTime = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - timeBegin
	);
}

std::chrono::milliseconds ToxTunCore::iterationInterval() noexcept {
	if (connections.empty()) {
		return std::chrono::milliseconds(tox_iteration_interval(tox));
	} else {
		/*
		 * We are assuming here (and in iterate()),
		 * that tox_interation_interval() is returing 50ms as an lower limit
		 * as it does at time of writing.
		 *
		 * TODO Maybe it would be a good idea to check if there is a busy
		 * connection and decrease the returned time if so.
		 */
		const auto tmp = std::chrono::milliseconds(5) - iterationTime;
		if (tmp.count() >= 0) {
			return tmp;
		} else {
			return std::chrono::milliseconds::zero();
		}
	}
}

void ToxTunCore::handleConnectionRequest(uint32_t friendNumber) noexcept {
	Logger::debug("ConnectionRequest received from ", friendNumber);

	if (!callbackFunction) {
		Logger::error("Callback function must be set to handle connection requests");
		return;
	}

	try {
		connections.emplace(
				std::piecewise_construct,
				std::forward_as_tuple(friendNumber), 
				std::forward_as_tuple(friendNumber, *this, false)
		);
	} catch (ToxTunError &error) {
		connections.erase(friendNumber); //needed?
		Connection::resetConnection(friendNumber, tox);
		return;
	}

	callbackFunction(
			ToxTun::Event::ConnectionRequested,
			friendNumber,
			callbackUserData
	);
}

void ToxTunCore::sendConnectionRequest(uint32_t friendNumber) {
	if (connections.count(friendNumber)) {
		throw ToxTunError("You have allready an open connection to this friend");
	} else {
		connections.emplace(
				std::piecewise_construct,
				std::forward_as_tuple(friendNumber), 
				std::forward_as_tuple(friendNumber, *this, true)
		);
	}
}

void ToxTunCore::acceptConnection(uint32_t friendNumber) {
	if (connections.count(friendNumber)) {
		try {
			connections.at(friendNumber).acceptConnection();
		} catch (ToxTunError &error) {
			connections.erase(friendNumber);
			throw ToxTunError(error);
		}
	} else {
		throw ToxTunError("No connection to accept from this friend");
	}
}

void ToxTunCore::rejectConnection(uint32_t friendNumber) noexcept {
	if (connections.erase(friendNumber) == 0) {
		Logger::debug("No connection to reject from this friend");
	}
}

void ToxTunCore::closeConnection(uint32_t friendNumber) noexcept {
	deleteConnection(friendNumber);
	Logger::debug("Closing connection to ", friendNumber);
}

ToxTun::ConnectionState ToxTunCore::getConnectionState(uint32_t friendNumber) noexcept {
	if (!connections.count(friendNumber)) {
		return ToxTun::ConnectionState::Disconnected;
	} else {
		return connections.at(friendNumber).getConnectionState();
	}
}

void ToxTunCore::deleteConnection(uint32_t friendNumber) noexcept {
	if (connections.erase(friendNumber) == 0) {
		Logger::debug("No connection to delete for this friend");
	}
}

Tox* ToxTunCore::getTox() const noexcept {
	return tox;
}

void ToxTunCore::callback(ToxTun::Event event, uint32_t friendNumber) const {
	if (!callbackFunction) throw ToxTunError("Callback function not set");

	callbackFunction(event, friendNumber, callbackUserData);
}
