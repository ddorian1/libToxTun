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

#include "ToxTun.hpp"

#include <map>
#include <tox/tox.h>

class Data;
class Connection;

/** 
 * This is the main backend class.
 * All public functions (except the constructor) should not throw an exception.
 */
class ToxTunCore : public ToxTun {
	private:
		Tox *tox; /**< Tox struct passed to ToxTun::ToxTun() */

		/**
		 * Connections
		 */
		std::map<uint32_t, Connection> connections;

		/**
		 * User Data to be returned by the callback function
		 */
		void *callbackUserData;

		/**
		 * Callback function to be called in case of an event
		 */
		ToxTun::CallbackFunction callbackFunction;

		/**
		 * Callback function to be registered to tox
		 */
		static void toxPacketCallback(
				Tox *tox, uint32_t friendNumber,
				const uint8_t *dataRaw,
				size_t length,
				void *ToxTunCoreVoid
		) noexcept ;

		/**
		 * Handles incoming packats
		 */
		void handleData(const Data &data, uint32_t friendNumber) noexcept ;

		/**
		 * Called by handleData
		 * \sa handleData
		 */
		void handleConnectionRequest(uint32_t friendNumber) noexcept ;

	public:
		/**
		 * Creates the tun interface and registers the callback functions
		 * to tox.
		 * \param[in] tox Pointer to Tox
		 */
		ToxTunCore(Tox *tox) noexcept;

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
		virtual void setCallback(ToxTun::CallbackFunction callback, void *userData = nullptr) noexcept final;

		/**
		 * This is doing the work.
		 * iterate() should be called in the main loop allongside with
		 * tox_iterate().
		 */
		virtual void iterate() noexcept final;

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
		virtual void rejectConnection(uint32_t friendNumber) noexcept final;

		/**
		 * Close connection to friend.
		 */
		virtual void closeConnection(uint32_t friendNumber) noexcept final;

		/**
		 * Delete connection to friend.
		 */
		void deleteConnection(uint32_t friendNumber) noexcept;

		/**
		 * Get pointer to Tox instance that was passed to the constructor.
		 */
		Tox* getTox() const noexcept;

		/**
		 * Call the callback function if it is set.
		 * Throws Error if callback function is not set.
		 */
		void callback(ToxTun::Event, uint32_t friendNumber) const;
};

#endif //TOX_TUN_CORE_HPP
