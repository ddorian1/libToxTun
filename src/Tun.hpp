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

#ifndef TUN_HPP
#define TUN_HPP

/** \file */

#include <cstdint>
#include <string>
#include <vector>
#include <list>
#include <array>

class Data;
class Tox;

/**
 * Abstract Tun interface class
 */
class TunInterface {
	private:
		const uint16_t toxUdpPort; /**< UDP port used by local tox instance */

		/**
		 * Check wether or not an ethernet frame is send from the own tox instance
		 */
		bool isFromOwnTox(const Data &data) noexcept;

		/**
		 * Called by isFromOwnTox()
		 * \sa isFromOwnTox()
		 */
		bool isFromOwnToxIPv4(const Data &data) noexcept;

		/**
		 * Called by isFromOwnTox()
		 * \sa isFromOwnTox()
		 */
		bool isFromOwnToxIPv6(const Data &data) noexcept;

	protected:
		/**
		 * Generate IPv4 Address from postfix.
		 * \param[in] postfix postfix to use
		 * \return string of form "192.168.<subnet>.<postfix>"
		 */
		static std::string ipv4FromPostfix(uint8_t subnet, uint8_t postfix) noexcept;

		/**
		 * Get data from tun interface.
		 * Called by getData()
		 * \sa getData()
		 */
		virtual Data getDataBackend() = 0;

		virtual std::list<std::array<uint8_t, 4>> getUsedIp4Addresses() = 0;

	public:
		TunInterface(const Tox *tox);

		TunInterface(const TunInterface&) = delete; /**< Deleted */
		TunInterface& operator=(const TunInterface&) = delete; /**< Deleted */

		/**
		 * Set IPv4 and IPv6 of tun interface
		 */
		virtual void setIp(uint8_t subnet, uint8_t postfix) noexcept = 0;

		/**
		 * Indicates wether or not there is pending data to be
		 * read from tun interface.
		 * \return true if there is pending data, false otherwise
		 */
		virtual bool dataPending() = 0;

		/**
		 * Get data from tun interface.
		 * May ether throw an error or lock if there isn't any data to read.
		 * \sa dataPending()
		 */
		Data getData();

		/**
		 * Send data to tun interface.
		 * Throws an error in case of failure.
		 */
		virtual void sendData(const Data &data) = 0;

		/**
		 * Wether or not the addressspace 192.168.<addrSpace>.0 is allready used.
		 * Throws an error if the addresses can't be determined.
		 */
		bool isAddrspaceUnused(uint8_t addrSpace);
};

#ifdef __unix
class TunUnix;
using Tun = TunUnix;
#include "TunUnix.hpp"
#elif _WIN32
class TunWin;
using Tun = TunWin;
#include "TunWin.hpp"
#else
#error No tun backend avaible for target platform
#endif


#endif //TUN_HPP
