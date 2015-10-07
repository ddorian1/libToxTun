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

#ifndef TOX_TUN_CORE_HPP
#define TOX_TUN_CORE_HPP

/** \file */

#include "Tun.hpp"
#include "ToxTun.hpp"

#include <list>
#include <map>
#include <tox/tox.h>
#include <vector>

class Error;
class Data;

/** 
 * This is the main backend class.
 * All public functions (except the constructor) should not throw an exception.
 */
class ToxTunCore : public ToxTun {
	private:
		/**
		 * Possible States
		 */
		enum class State {
			Idle,
			RequestPending, 
			ExpectingIpPacket,
			Connected,
			PermanentError
		};

		Tox *tox; /**< Tox struct passed to ToxTun::ToxTun() */
		Tun tun; /**< Tun interface */
		State state; /**< Current state */

		/**
		 * Fragments of jet incomplete received packages.
		 */
		std::map<uint32_t, std::map<uint8_t, std::list<Data> > > fragments;

		/**
		 * User Data to be returned by the callback function
		 */
		void *callbackUserData;

		/**
		 * Callback function to be called in case of an event
		 */
		ToxTun::CallbackFunction callbackFunction;

		/**
		 * Friend we are connected to.
		 * The value is unspecified if state == State::Idle.
		 */
		uint32_t connectedFriend; 

		/**
		 * Callback function to be registered to tox
		 */
		static void toxPacketCallback(
				Tox *tox, uint32_t friendNumber,
				const uint8_t *dataRaw,
				size_t length,
				void *ToxTunCoreVoid
		);

		/**
		 * Handles incoming packats
		 */
		void handleData(const Data &data, uint32_t friendNumber);

		/**
		 * Called by handleData
		 * \sa handleData
		 */
		void handleFragment(const Data &data, uint32_t friendNumber);

		/**
		 * Called by handleData
		 * \sa handleData
		 */
		void handleConnectionRequest(uint32_t friendNumber);

		/**
		 * Called by handleData
		 * \sa handleData
		 */
		void handleConnectionAccepted(uint32_t friendNumber);

		/**
		 * Called by handleData
		 * \sa handleData
		 */
		void handleConnectionRejected(uint32_t friendNumber);

		/**
		 * Called by handleData
		 * \sa handleData
		 */
		void handleConnectionClosed(uint32_t friendNumber);

		/**
		 * Called by handleData
		 * \sa handleData
		 */
		void handleConnectionReset(uint32_t friendNumber);

		/**
		 * Called by handleData
		 * \param[in] data Data received via tox
		 * \param[in] friendNumber Sender of the data
		 * \sa handleData
		 */
		void setIp(const Data &data, uint32_t friendNumber);

		/**
		 * Resets the connection in case something unexpected happens
		 * \param[in] friendNumber Friend to whom the connection should be reset
		 */
		void resetConnection(uint32_t friendNumber);

		/**
		 * Send data via tun interface
		 */
		void sendToTun(const Data &data, uint32_t friendNumber);

		/**
		 * Send data to friend via Tox
		 */
		void sendToTox(const Data &data, uint32_t friendNumber) const;

		/**
		 * Handle exceptions
		 * \param[in] error Instance of Error
		 * \param[in] sensitiv Wether or not we are in an error sensitiv context
		 */
		void handleError(Error &error, bool sensitiv = false);

	public:
		/**
		 * Creates the tun interface and registers the callback functions
		 * to tox.
		 * \param[in] tox Pointer to Tox
		 */
		ToxTunCore(Tox *tox);

		ToxTunCore(const ToxTunCore&) = delete; /**< Deleted */
		ToxTunCore& operator=(const ToxTunCore&) = delete; /**< Deleted */

		/**
		 * Closes the tun interface and unregisters the callback functions
		 * from tox.
		 */
		virtual ~ToxTunCore();

		/**
		 * Sets the callback function that is called for new events.
		 * \param[in] callback Pointer to the callback function. Pass
		 * nullptr to remove it.
		 * \param[in] userData Possible data that is passed 
		 * to the callback function at each call.
		 */
		virtual void setCallback(ToxTun::CallbackFunction callback, void *userData = nullptr) final;

		/**
		 * This is doing the work.
		 * iterate() should be called in the main loop allongside with
		 * tox_iterate().
		 */
		virtual void iterate() final;

		/**
		 * Sends an connection Request to the friend.
		 */
		virtual void sendConnectionRequest(uint32_t friendNumber) final;

		/**
		 * Accepts an priviously received connection request from friend.
		 */
		virtual void acceptConnection(uint32_t friendNumber) final;

		/**
		 * Rejects an priviously received connection request from friend.
		 */
		virtual void rejectConnection(uint32_t friendNumber) final;

		/**
		 * Close connection to friend.
		 * This also unsets the ip of 
		 * the tun interface.
		 */
		virtual void closeConnection() final;
};

#endif //TOX_TUN_CORE_HPP
