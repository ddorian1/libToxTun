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

#ifndef DATA_HPP
#define DATA_HPP

/** \file */

#include <cstdint>
#include <cstring>
#include <forward_list>
#include <list>
#include <vector>
#include <memory>

/**
 * Class for convenient handling of data to send or receive.
 */
class Data {
	public:
		/**
		 * The packet identifier for packets send trough tox.
		 * For lossless packets it must be in range 160-191,
		 * for lossy packets in 200-254.
		 */
		enum class PacketId : uint8_t {
			ConnectionRequest = 160,
			ConnectionAccept = 161,
			ConnectionReject = 162,
			ConnectionClose = 163,
			ConnectionReset = 164,
			IP = 165,
			Data = 200,
			Fragment = 201
		};

		/**
		 * How to send the packet via tox.
		 */
		enum class SendTox {
			Lossless,
			Lossy
		};

	private:
		/**
		 * The actuall data.
		 * The first byte is reserved for the tox header.
		 */
		std::shared_ptr< std::vector<uint8_t> > data;

		/**
		 * Identifice wether or not the first byte of the data is an 
		 * valid tox header.
		 */
		bool toxHeaderSet;

		/**
		 * The index for the next splitted data packet
		 */
		static uint8_t splittedDataIndex;

		/**
		 * Private Constructor.
		 * Use the static members to create an instance.
		 */
		Data(size_t len);

	public:
		/**
		 * Create class from data received via Tun interface.
		 * len must be the size of buffer.
		 */
		static Data fromTunData(const uint8_t *buffer, size_t len);

		/**
		 * Create class from data received via Tox.
		 * len must be the size of buffer.
		 */
		static Data fromToxData(const uint8_t *buffer, size_t len);

		/**
		 * Create class from list of fragments received via Tox
		 */
		static Data fromFragments(std::list<Data> &fragments);

		/**
		 * Create class from an ip postfix.
		 * Sets the header to Data::PacketId::IP.
		 */
		static Data fromIpPostfix(uint8_t postfix);

		/**
		 * Create class from an Data::PacketId.
		 * This only sets the header without any additional data.
		 */
		static Data fromPacketId(PacketId id);

		/**
		 * Changes the header to the given one.
		 */
		void setToxHeader(PacketId id);

		/**
		 * Returns the header.
		 */
		PacketId getToxHeader() const;

		/**
		 * Gets the data to send via the tun interface.
		 * This is Data::data without the first byte.
		 */
		const uint8_t *getIpData() const;

		/**
		 * Gets the size of the buffer returned by getIpData().
		 */
		size_t getIpDataLen() const;

		/**
		 * Gets the data to send via tox.
		 * This is the content of Data::data.
		 */
		const uint8_t *getToxData() const;

		/**
		 * Gets the size of the buffer returned by getToxData().
		 */
		size_t getToxDataLen() const;

		/**
		 * Gets the ip postfix of a ip packet received via tox.
		 * Throws an error, if header isn't Data::PacketId::IP
		 * or is otherwise invalid.
		 */
		uint8_t getIpPostfix() const;

		/**
		 * Gets the set index from a fragment packet.
		 */
		uint8_t getSplittedDataIndex() const;

		/**
		 * Gets the count of fragments in the set from a fragment packet.
		 */
		uint8_t getFragmentsCount() const;

		/**
		 * Gets a list of splitted packegs that fit TOX_MAX_CUSTOM_PACKAGE_SIZE.
		 */
		std::forward_list<Data> getSplitted() const;

		/**
		 * Gets the type of connection the packet must be send over via tox.
		 */
		SendTox getSendTox() const;
};

#endif //DATA_HPP

