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
		bool isFromOwnTox(const Data &data);

		/**
		 * Called by isFromOwnTox()
		 * \sa isFromOwnTox()
		 */
		bool isFromOwnToxIPv4(const Data &data);

		/**
		 * Called by isFromOwnTox()
		 * \sa isFromOwnTox()
		 */
		bool isFromOwnToxIPv6(const Data &data);

	protected:
		/**
		 * Generate IPv4 Address from postfix.
		 * \param[in] postfix postfix to use
		 * \return string of form "10.0.0.[postfix]"
		 */
		static std::string ipv4FromPostfix(const uint8_t postfix);

		/**
		 * Get data from tun interface.
		 * Called by getData()
		 * \sa getData()
		 */
		virtual Data getDataBackend() = 0;

	public:
		TunInterface(const Tox *tox);

		/**
		 * Set IPv4 and IPv6 of tun interface
		 */
		virtual void setIp(const uint8_t postfix) = 0;

		/**
		 * Bring tun interface down
		 */
		virtual void unsetIp() = 0;

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
