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

#ifndef TUN_WIN_HPP
#define TUN_WIN_HPP

/** \file */

#ifdef _WIN32

#include "Tun.hpp"

#include <list>
#include <string>
#include <windows.h>

/**
 * The windows backend for TunInterface
 */
class TunWin : public TunInterface {
	private:
		enum class ReadState {
			Idle,
			Queued,
			Ready
		};

		HANDLE handle; /**< handle of tun interface */
		std::string devGuid; /**< guid of tun interface */
		ULONG ipApiContext; /**< NTE context returned by AddIpAddress */
		uint8_t ipPostfix; /**< acutall ip postfix, 255 if no ip is set */
		uint8_t readBuffer[65535 + 40]; /**< buffer for data read from tun */
		// 65535+40 == max length of an IP packet
		DWORD bytesRead; /**< bytes in readBuffer if readState==ReadState::Ready */
		ReadState readState; /**< State we are in */
		OVERLAPPED overlappedRead; /**< for reading async */
		std::list<OVERLAPPED> overlappedWrite; /**< for writeing async */
		bool ipIsSet; /**< wether or not the IP was set successfully */

		/**
		 * Queues a new async read
		 */
		void queueRead();

		/**
		 * Sets bytesRead, using the info from overlappedRead
		 */
		void setBytesRead();

	public:
		/**
		 * Creates the tun interface
		 */
		TunWin();

		TunWin(const TunWin&) = delete; /**< Deleted */
		TunWin& operator=(const TunWin&) = delete; /**< Deleted */

		/**
		 * Closes the tun interface
		 */
		~TunWin();

		virtual void setIp(const uint8_t postfix) final;
		virtual void unsetIp() final;
		virtual bool dataPending() final;
		virtual Data getData() final;
		virtual void sendData(const Data &data) final;
};

#undef ERROR //qTox has a conflicting enum, so undef it for now

#endif //_WIN32

#endif //TUN_WIN_HPP
