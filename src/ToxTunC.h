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
enum CCallbackEvents {
	connection_requested,
	connection_accepted,
	connection_rejected,
	connection_closed
};

/**
 * Creates a new ToxTun class.
 * \sa ToxTun::ToxTun().
 * \param[in] tox Pointer to tox
 * \return Pointer to the created ToxTun class
 */
void *toxtun_new(Tox *tox);

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
			enum CCallbackEvents event,
			uint32_t friendNumber,
			void *userData
		),
		void *userData
);

/**
 * Do the work.
 * \sa ToxTun::iterate()
 */
void toxtun_iterate(void *toxtun);

/**
 * Sends a connection request to friend.
 * \sa ToxTun::sendConnectionRequest()
 */
void toxtun_send_connection_request(void *toxtun, uint32_t friendNumber);

/**
 * Accept connection request from friend.
 * \sa ToxTun::acceptConnection()
 */
void toxtun_accept_connection(void *toxtun, uint32_t friendNumber);

/**
 * Reject connection request.
 * \sa ToxTun::rejectConnection()
 */
void toxtun_reject_connection(void *toxtun, uint32_t friendNumber);

/**
 * Close connection to friend.
 * \sa ToxTun::closeConnection()
 */
void toxtun_close_connection(void *toxtun);

/**\}*/

#ifdef __cplusplus
} //extern "C"
#endif

#endif //TOX_TUN_H_C
