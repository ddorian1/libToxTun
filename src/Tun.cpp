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

#include "Data.hpp"
#include "Logger.hpp"
#include "Tun.hpp"
#include "ToxTun.hpp"

#include <sstream>
#include <tox/tox.h>

TunInterface::TunInterface(const Tox *tox)
:
	toxUdpPort(tox_self_get_udp_port(tox, nullptr))
{}

std::string TunInterface::ipv4FromPostfix(const uint8_t postfix) noexcept {
	std::ostringstream ip;
	
	ip << "10.0.0." << static_cast<int>(postfix);
	return ip.str();
}

Data TunInterface::getData() {
	Data data = getDataBackend();

	if (isFromOwnTox(data)) {
		throw ToxTunError("Dropping packet from own tox instance");
	}

	return data;
}

bool TunInterface::isFromOwnTox(const Data &data) noexcept {
	if (data.getIpDataLen() < 14) return false;

	const uint8_t *tmp;
	try {
		tmp = data.getIpData();
	} catch (ToxTunError &error) {
		return false;
	}

	if (tmp[12] == 0x08 && tmp[13] == 0x00)
		return isFromOwnToxIPv4(data);

	if (tmp[12] == 0x86 && tmp[13] == 0xDD)
		return isFromOwnToxIPv6(data);

	return false;
}

bool TunInterface::isFromOwnToxIPv4(const Data &data) noexcept {
	constexpr uint8_t etherFrameOffset = 14;

	if (data.getIpDataLen() < etherFrameOffset + 10u) return false;

	const uint8_t *tmp;
	try {
		tmp = data.getIpData();
	} catch (ToxTunError &error) {
		return false;
	}

	if (tmp[etherFrameOffset + 9] != 0x11) return false; //No UDP

	if ((tmp[etherFrameOffset + 6] & 0x20) != 0) { //Fragmented
		uint8_t f = (tmp[etherFrameOffset + 6] & 0x2F) | tmp[etherFrameOffset + 7];
		if (f) return false; //Not first fragment, so no way to check source port
	}

	const uint8_t ipHeaderLength = (tmp[etherFrameOffset] & 0x0F) * 4;
	const uint8_t ipDataOffset = etherFrameOffset + ipHeaderLength;
	if (data.getIpDataLen() < ipDataOffset + 2u) return false;

	uint16_t port = static_cast<uint16_t>(tmp[ipDataOffset]) << 8 | tmp[ipDataOffset + 1];

	if (port == toxUdpPort) return true;

	return false;
}

bool TunInterface::isFromOwnToxIPv6(const Data &data) noexcept {
	constexpr uint8_t etherFrameOffset = 14;
	uint8_t ipDataOffset = etherFrameOffset + 40;

	if (data.getIpDataLen() < ipDataOffset) return false;
	const uint8_t *tmp;
	try {
		tmp = data.getIpData();
	} catch (ToxTunError &error) {
		return false;
	}

	//TODO This may also be another extension header, so deal with it
	if (tmp[etherFrameOffset + 6] == 44u) { //Fragment
		if (data.getIpDataLen() < ipDataOffset + 10u) return false;
		uint8_t f = tmp[etherFrameOffset + 42] | (tmp[etherFrameOffset + 43] & 0xF8);
		if (f) {
			return false; //Not first fragment, so no way to check source port
		} else {
			//TODO This may also be another extension header, so deal with it
			if (tmp[etherFrameOffset + 40] != 0x11) return false; //No UDP
			ipDataOffset += 8;
		}
	} else {
		if (tmp[etherFrameOffset + 6] != 0x11) return false; //No UDP
	}

	uint16_t port = static_cast<uint16_t>(tmp[ipDataOffset]) << 8 | tmp[ipDataOffset + 1];

	if (port == toxUdpPort) return true;

	return false;
}
