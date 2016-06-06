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

#ifndef TOX_TUN_C_H
#define TOX_TUN_C_H

/** \file */

#ifdef __cplusplus
#include <cstdint>
extern "C" {
#else
#include <stdint.h>
#endif

class Tox;

/**
 * \defgroup C_API C API
 * \{
 */

/**
 * Possible events for the callback function
 * \sa toxtun_set_callback()
 */
enum toxtun_event {
	TOXTUN_CONNECTION_REQUESTED,
	TOXTUN_CONNECTION_ACCEPTED,
	TOXTUN_CONNECTION_REJECTED,
	TOXTUN_CONNECTION_CLOSED
};


/**
 * Possible connection states
 * \sa toxtun_get_connection_state()
 */
enum toxtun_connection_state {
	TOXTUN_CONNECTION_STATE_CONNECTED,
	TOXTUN_CONNECTION_STATE_DISCONNECTED,
	TOXTUN_CONNECTION_STATE_RINGING_AT_FRIEND,
	TOXTUN_CONNECTION_STATE_FRIEND_IS_RINGING
};

/**
 * Creates a new ToxTun class.
 * \sa ToxTun::ToxTun().
 * \param[in] tox Pointer to tox
 * \return Pointer to the created ToxTun class, NULL in case of error
 */
void* toxtun_new(Tox *tox);

/**
 * Destructs ToxTun and frees the memory.
 * \sa ToxTun::~ToxTun().
 */
void toxtun_kill(void *toxtun);

/**
 * Sets the callback function.
 * \sa ToxTun::setCallback()
 */
void toxtun_set_callback(
		void *toxtun,
		void (*cb)(
			enum toxtun_event event,
			uint32_t friendNumber,
			void *userData
		),
		void *userData
);

/**
 * Does the work.
 * \sa ToxTun::iterate()
 */
void toxtun_iterate(void *toxtun);

/**
 * Time till tox_iterate and toxtun_iterate should be called the next time.
 * \sa ToxTun::iterationInterval()
 */
unsigned int toxtun_iteration_interval(void *toxtun);

/**
 * Sends a connection request to friend.
 * \return false in case of error, true otherwise
 * \sa getLastError()
 * \sa ToxTun::sendConnectionRequest()
 */
bool toxtun_send_connection_request(void *toxtun, uint32_t friendNumber);

/**
 * Accept connection request from friend.
 * \return false in case of error, true otherwise
 * \sa getLastError()
 * \sa ToxTun::acceptConnection()
 */
bool toxtun_accept_connection(void *toxtun, uint32_t friendNumber);

/**
 * Reject connection request.
 * \sa ToxTun::rejectConnection()
 */
void toxtun_reject_connection(void *toxtun, uint32_t friendNumber);

/**
 * Close connection to friend.
 * \sa ToxTun::closeConnection()
 */
void toxtun_close_connection(void *toxtun, uint32_t friendNumber);

/**
 * Get current state of connection to friend.
 */
enum toxtun_connection_state toxtun_get_connection_state(void *toxtun, uint32_t friendNumber);

/**
 * Get humen readable description of last error.
 * \return Pointer to string, vaild until next call to get_last_error with the same toxtun instance as argument.
 */
const char* toxtun_get_last_error(void *toxtun);

/**\}*/

#ifdef __cplusplus
} //extern "C"
#endif

#endif //TOX_TUN_H_C
