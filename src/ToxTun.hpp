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

#ifndef TOX_TUN_HPP
#define TOX_TUN_HPP

/** \file */

#include <cstdint>
#include <exception>
#include <string>
#include <memory>

class ToxTunCore;
class Tox;

/** 
 * \mainpage 
 * A library to create virtual networks with friend via tox.
 * \n
 * For the C++ API see ToxTun,
 * for the C API see \ref C_API "here".
 */

/** 
 * The main API class.
 * This is the class to instanciate to create a ToxTun instance.
 * All functions except newToxTun() shouldn't thow an error.
 * If something goes wrong, the callback function is called with
 * Event::ConnectionClosed. This will probably change to another
 * event in the future.
 */
class ToxTun {
	public:
		/**
		 * Possible events for the callback function
		 * \sa CallbackFunction
		 */
		enum class Event {
			ConnectionRequested,
			ConnectionAccepted,
			ConnectionRejected,
			ConnectionClosed
		};

		/**
		 * Type for the callback function
		 * \sa setCallback()
		 */
		using CallbackFunction = void (*)(
				Event event, 
				uint32_t friendNumber,
				void *userData
		);

		ToxTun() = default;
		virtual ~ToxTun() = default;

		ToxTun(const ToxTun&) = delete; /**< Deleted */
		ToxTun& operator=(const ToxTun&) = delete; /**< Deleted */

		/**
		 * Creates a new ToxTun class.
		 * Throws ToxTunError in case of failure.
		 * \param[in] tox Pointer to Tox
		 * \return std::shared_ptr to created class
		 * \sa newToxTunNoExp()
		 */
		static std::shared_ptr<ToxTun> newToxTun(Tox *tox);

		/**
		 * Creates a new ToxTun class without throwing an exception.
		 * The caller is responsible for freeing the memory!
		 * \param[in] tox Pointer to Tox
		 * \return nullptr in case of error
		 * \sa newToxTun()
		 */
		static ToxTun* newToxTunNoExp(Tox *tox);

		/**
		 * Sets the callback function that is called for new events.
		 * \param[in] callback Pointer to the callback function. Pass
		 * nullptr to remove it.
		 * \param[in] userData Possible data that is passed 
		 * to the callback function at each call.
		 */
		virtual void setCallback(CallbackFunction callback, void *userData = nullptr) = 0;

		/**
		 * This is doing the work.
		 * iterate() should be called in the main loop allongside with
		 * tox_iterate().
		 */
		virtual void iterate() = 0;

		/**
		 * Sends an connection Request to the friend.
		 */
		virtual void sendConnectionRequest(uint32_t friendNumber) = 0;

		/**
		 * Accepts an priviously received connection request from friend.
		 */
		virtual void acceptConnection(uint32_t friendNumber) = 0;

		/**
		 * Rejects an priviously received connection request from friend.
		 */
		virtual void rejectConnection(uint32_t friendNumber) = 0;

		/**
		 * Close connection to friend.
		 * This also unsets the ip of 
		 * the tun interface.
		 */
		virtual void closeConnection() = 0;
};

/**
 * Error class thrown by newToxTun()
 */
class ToxTunError : public std::exception {
	private:
		std::string string;

	public:
		ToxTunError(const std::string &string) : string(string) {}
		virtual const char* what() const noexcept {return string.c_str();}
};

#endif //TOX_TUN_HPP

