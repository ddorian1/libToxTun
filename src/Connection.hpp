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

#ifndef TOX_TUN_CONNECTION_HPP
#define TOX_TUN_CONNECTION_HPP

/** \file */

#include "Tun.hpp"
#include "ToxTun.hpp"

#include <list>
#include <map>
#include <chrono>

class Data;
class Tox;
class ToxTunCore;

/** 
 * This is the main backend class.
 * All public functions (except the constructor) should not throw an exception.
 */
class Connection {
	private:
		/**
		 * Possible states
		 * This states are only used inside this class
		 */
		enum class State {
			OwnRequestPending, 
			FriendsRequestPending, 
			ExpectingIpPacket,
			ExpectingIpConfirmation,
			Connected,
			Deleting
		};

		ToxTunCore &toxTunCore; /**< ToxTunCore */
		Tun tun; /**< Tun interface */
		State state; /**< Current state */

		/**
		 * Fragments of jet incomplete received packages.
		 */
		std::map<uint8_t, std::list<Data> > fragments;

		/**
		 * Friend this connected is to.
		 */
		const uint32_t connectedFriend; 

		/**
		 * Index for next fragmented package to send.
		 */
		uint8_t nextFragmentIndex;

		/**
		 * Last subnet we have proposed to friend.
		 * -1 if we havend done so yet.
		 */
		int16_t subnet;

		/**
		 * Called by handleData
		 * \sa handleData
		 */
		void handleFragment(const Data &data) noexcept;

		/**
		 * Called by handleData
		 * \sa handleData
		 */
		void handleConnectionRequest();

		/**
		 * Called by handleData
		 * \sa handleData
		 */
		void handleConnectionAccepted() noexcept;

		/**
		 * Called by handleData
		 * \sa handleData
		 */
		void handleConnectionRejected() noexcept;

		/**
		 * Called by handleData
		 * \sa handleData
		 */
		void handleConnectionClosed() noexcept;

		/**
		 * Called by handleData
		 * \sa handleData
		 */
		void handleConnectionReset() noexcept;

		/**
		 * Called by handleData
		 * \param[in] data Data received via tox
		 * \sa handleData
		 */
		void handleIpProposal(const Data &data) noexcept;

		/**
		 * Called by handleData
		 * \sa handleData
		 */
		void handleIpAccepted() noexcept;

		/**
		 * Called by handleData
		 * \sa handleData
		 */
		void handleIpRejected() noexcept;

		/**
		 * Reset the connection without deleting it
		 * \sa resetAndDeleteConnection()
		 */
		void resetConnection() noexcept;

		/**
		 * Reset and delete the connection in case something went wrong
		 * \sa resetConnection()
		 */
		void resetAndDeleteConnection() noexcept;

		/**
		 * Send a connection request to the friend
		 */
		void sendConnectionRequest();

		/**
		 * Close the connection
		 */
		void closeConnection() noexcept;

		/**
		 * Reject pending connection request
		 */
		void rejectConnection() noexcept;

		/**
		 * Send Ip proposal to friend
		 */
		void sendIp() noexcept;

		/**
		 * Set Ip and change state to connected
		 */
		void setIp(uint8_t subnet, uint8_t postfix) noexcept;

		/**
		 * Delete the connection from ToxTunCore
		 */
		void deleteConnection() noexcept;

		/**
		 * Send data via tun interface
		 */
		void sendToTun(const Data &data) noexcept;

		/**
		 * Send data to friend via Tox
		 */
		void sendToTox(const Data &data);

	public:
		/**
		 * Creates the tun interface and registers the callback functions
		 * to tox.
		 * \param[in] tox Pointer to Tox
		 */
		Connection(uint32_t friendNumber, ToxTunCore &toxTunCore, bool initiate);

		Connection(const Connection&) = delete; /**< Deleted */
		Connection& operator=(const Connection&) = delete; /**< Deleted */

		/**
		 * Closes the tun interface and unregisters the callback functions
		 * from tox.
		 */
		~Connection();

		/**
		 * This is doing the work.
		 */
		void iterate(std::chrono::duration<double> time) noexcept;

		/**
		 * Handles incoming packats
		 */
		void handleData(const Data &data) noexcept;

		/**
		 * Accepts an priviously received connection request from friend.
		 */
		void acceptConnection();

		/**
		 * Resets the connection in case something unexpected happens
		 * \param[in] friendNumber Friend to whom the connection should be reset
		 */
		static void resetConnection(uint32_t friendNumber, Tox *tox) noexcept;

		/**
		 * Send data to given friend via tox
		 */
		static void sendToTox(
				const Data &data,
				uint32_t friendNumber,
				Tox *tox,
				uint8_t *nextFragmentIndex = nullptr
		);

		/**
		 * Get current state of connection to friend.
		 */
		ToxTun::ConnectionState getConnectionState() noexcept;
};

#endif //TOX_TUN_CONNECTION_HPP
