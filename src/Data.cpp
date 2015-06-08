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
#include "Error.hpp"

Data::Data(size_t len) 
:
	data(len),
	toxHeaderSet(false)
{}

Data Data::fromToxData(const uint8_t *buffer, size_t len) {
	Data data(len);
	memcpy(&(data.data[0]), buffer, len);
	data.toxHeaderSet = true;

	return data;
}

Data Data::fromToxData(const std::list<Data> &fragments) {
	size_t len = 1;
	for (const auto &f : fragments) len += f.data.size() - 1;

	Data data(len);

	size_t pos = 1;
	for (const auto &f : fragments) {
		memcpy(&(data.data[pos]), &(f.data[1]), f.data.size()-1);
		pos += f.data.size()-1;
	}

	data.setToxHeader(PacketId::DataEnd);

	return data;
}

Data Data::fromTunData(const uint8_t *buffer, size_t len, bool fragment) {
	Data data(len + 1);
	memcpy(&(data.data[1]), buffer, len);
	if (fragment) {
		data.setToxHeader(PacketId::DataFrag);
	} else {
		data.setToxHeader(PacketId::DataEnd);
	}
	
	return data;
}

Data Data::fromIpPostfix(uint8_t postfix) {
	Data data(2);
	data.data[1] = postfix;
	data.setToxHeader(PacketId::IP);

	return data;
}

Data Data::fromPacketId(PacketId id) {
	Data data(1);
	data.setToxHeader(id);

	return data;
}

void Data::setToxHeader(PacketId id) {
	data.at(0) = static_cast<uint8_t>(id);
	toxHeaderSet = true;
}

Data::PacketId Data::getToxHeader() const {
	if (!toxHeaderSet) {
		//This should never happen
		Logger::error("ToxHeader not set for Data.");
		throw Error(Error::Err::Critical);
	}
	
	return static_cast<PacketId>(data.at(0));
}
	
const uint8_t* Data::getIpData() const {
	return &(data[1]);
}

size_t Data::getIpDataLen() const {
	return data.size() - 1;
}

const uint8_t* Data::getToxData() const {
	if (!toxHeaderSet) {
		//This should never happen
		Logger::error("ToxHeader not set for Data");
		throw Error(Error::Err::Critical);
	}
	
	return &(data.at(0));
}

size_t Data::getToxDataLen() const {
	return data.size();
}

uint8_t Data::getIpPostfix() const {
	if (getToxHeader() != PacketId::IP) {
		Logger::error("Requesting IP from a non IP Packet");
		throw Error(Error::Err::Critical);
	}
	if (data.size() != 2) {
		Logger::error("Ip Packet has invalid size");
		throw Error(Error::Err::Critical);
	}

	return data[1];
}
