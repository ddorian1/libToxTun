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
#include "ToxTun.hpp"

#include <tox/tox.h>

Data::Data(size_t len) noexcept
:
	data(std::make_shared<std::vector<uint8_t>>(len ? len : 1)),
	toxHeaderSet(false)
{}

Data Data::fromToxData(const uint8_t *buffer, size_t len) noexcept {
	Data data(len);
	memcpy(&(data.data->at(0)), buffer, len);
	data.toxHeaderSet = true;

	return data;
}

Data Data::fromFragments(std::list<Data> &fragments) {
	size_t len = 0;
	for (const auto &f : fragments) len += f.data->size() - 4;

	Data data(len);

	fragments.sort(
			[](const Data &d1, const Data &d2) {
				return (d1.data->at(2) < d2.data->at(2));
			}
	);

	size_t pos = 0;
	size_t i = 0;
	for (const auto &f : fragments) {
		if (i != f.data->at(2)) {
			throw ToxTunError("Fragmented package corrupted");
		}
		++i;

		memcpy(&data.data->at(pos), &f.data->at(4), f.data->size() - 4);
		pos += f.data->size() - 4;
	}

	data.toxHeaderSet = true;

	return data;
}

Data Data::fromTunData(const uint8_t *buffer, size_t len) {
	if (len + 1 == 0) {
		throw ToxTunError("Data from Tun to long to store in vector");
	}

	Data data(len + 1);
	memcpy(&(data.data->at(1)), buffer, len);
	data.setToxHeader(PacketId::Data);
	
	return data;
}

Data Data::fromIpPostfix(uint8_t postfix) noexcept {
	Data data(2);
	data.data->at(1) = postfix;
	data.setToxHeader(PacketId::IP);

	return data;
}

Data Data::fromPacketId(PacketId id) noexcept {
	Data data(1);
	data.setToxHeader(id);

	return data;
}

void Data::setToxHeader(PacketId id) noexcept {
	data->at(0) = static_cast<uint8_t>(id);
	toxHeaderSet = true;
}

Data::PacketId Data::getToxHeader() const {
	if (!toxHeaderSet) {
		//This should never happen
		throw ToxTunError("ToxHeader not set for Data package (1)");
	}
	
	return static_cast<PacketId>(data->at(0));
}
	
const uint8_t* Data::getIpData() const {
	if (data->size() < 2) {
		//This should never happen
		throw ToxTunError("Trying to access IpData in Data packet of length 1");
	}
	return &(data->at(1));
}

size_t Data::getIpDataLen() const noexcept {
	return data->size() - 1;
}

const uint8_t* Data::getToxData() const {
	if (!toxHeaderSet) {
		//This should never happen
		throw ToxTunError("ToxHeader not set for Data package (2)");
	}
	
	return &(data->at(0));
}

size_t Data::getToxDataLen() const noexcept {
	return data->size();
}

uint8_t Data::getIpPostfix() const {
	if (getToxHeader() != PacketId::IP) {
		//This should never happen
		throw ToxTunError("Requesting IP from a non IP Packet");
	}
	if (data->size() != 2) {
		throw ToxTunError("Ip Packet has invalid size");
	}

	return data->at(1);
}

std::forward_list<Data> Data::getSplitted() const {
	size_t pos = 0;
	size_t fragmentIndex = 0;
	std::forward_list<Data> dataList;

	while (pos < data->size()) {
		size_t toCpy = (data->size() - pos < TOX_MAX_CUSTOM_PACKET_SIZE - 4) ?
			data->size() - pos : TOX_MAX_CUSTOM_PACKET_SIZE - 4;

		if (toCpy + 4 < toCpy) {
			//This should never happen
			throw ToxTunError("Integer overflow in getSplitted()");
		}

		Data tmp = Data(toCpy + 4);
		tmp.setToxHeader(PacketId::Fragment);
		tmp.data->at(1) = splittedDataIndex;
		tmp.data->at(2) = fragmentIndex;
		memcpy(&(tmp.data->at(4)), &(data->at(pos)), toCpy);

		pos += toCpy;
		++fragmentIndex;

		dataList.push_front(std::move(tmp));
	}

	for(auto &f : dataList) f.data->at(3) = fragmentIndex;

	splittedDataIndex = (splittedDataIndex == 255) ?
		0 : (splittedDataIndex + 1);

	return dataList;
}

Data::SendTox Data::getSendTox() const {
	const uint8_t h = static_cast<uint8_t>(getToxHeader());
	if (200 <= h && 254 >= h) {
		return SendTox::Lossy;
	} else if (160 <= h && 191 >= h) {
		return SendTox::Lossless;
	} else {
		//This should never happen
		throw ToxTunError("Called getSendTox(), but toxHeader not in range");
	}
}

bool Data::isValidFragment() const noexcept {
	PacketId id;
	try {
		id = getToxHeader();
	} catch (ToxTunError &error) {
		return false;
	}
	if (id != PacketId::Fragment) {
		Logger::debug("isValidFragment called on non fragment");
		return false;
	}

	if (data->size() < 4) {
		Logger::debug("Fragment to short");
		return false;
	}

	return true;
}

uint8_t Data::getSplittedDataIndex() const {
	if (getToxHeader() != PacketId::Fragment) {
		//This should never happen
		throw ToxTunError("Trying to get SplittedDataIndex from a non fragment");
	}
	if (data->size() < 2) {
		throw ToxTunError("Empty Data fragment");
	}
	return data->at(1);
}

uint8_t Data::getFragmentsCount() const {
	if (getToxHeader() != PacketId::Fragment) {
		throw ToxTunError("Trying to get fragmentsCount from a non fragment");
	}
	if (data->size() < 4) {
		throw ToxTunError("Data fragment to short");
	}
	return data->at(3);
}

uint8_t Data::splittedDataIndex = 0;
