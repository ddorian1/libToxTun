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

#ifndef TUN_UNIX_HPP
#define TUN_UNIX_HPP

/** \file */

#ifdef __unix

#include "Tun.hpp"

#include <string>
#include <list>
#include <array>

/**
 * The unix backend for TunInterface
 */
class TunUnix : public TunInterface {
	private:
		const int fd; /**< file desctiptor of tun interface */
		std::string name; /**< name of tun interface */

		void shutdown();
		virtual Data getDataBackend() final;
		virtual std::list<std::array<uint8_t, 4>> getUsedIp4Addresses() final;

	public:
		/**
		 * Creates the tun interface
		 */
		TunUnix(const Tox *tox);

		TunUnix(const TunUnix&) = delete; /**< Deleted */
		TunUnix& operator=(const TunUnix&) = delete; /**< Deleted */

		/**
		 * Closes the tun interface
		 */
		~TunUnix();

		virtual void setIp(uint8_t subnet, uint8_t postfix) noexcept final;
		virtual bool dataPending() final;
		virtual void sendData(const Data &data) final;
};

#endif //__unix

#endif //TUN_UNIX_HPP
