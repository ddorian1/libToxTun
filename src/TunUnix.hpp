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

/**
 * The unix backend for TunInterface
 */
class TunUnix : public TunInterface {
	private:
		const int fd; /**< file desctiptor of tun interface */
		std::string name; /**< name of tun interface */
		uint8_t ipPostfix; /**< acutall ip postfix, 255 if no ip is set */

	public:
		/**
		 * Creates the tun interface
		 */
		TunUnix();

		TunUnix(const TunUnix&) = delete; /**< Deleted */
		TunUnix& operator=(const TunUnix&) = delete; /**< Deleted */

		/**
		 * Closes the tun interface
		 */
		~TunUnix();

		virtual void setIp(const uint8_t postfix) final;
		virtual void unsetIp() final;
		virtual bool dataPending() final;
		virtual Data getData() final;
		virtual void sendData(const Data &data) final;
};

#endif //__unix

#endif //TUN_UNIX_HPP