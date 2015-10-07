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

#include "ToxTun.hpp"
#include "ToxTunCore.hpp"
#include "Error.hpp"
#include "Logger.hpp"

std::shared_ptr<ToxTun> ToxTun::newToxTun(Tox *tox) {
	std::shared_ptr<ToxTun> ptr;

	try {
		ptr.reset(new ToxTunCore(tox));
	} catch (Error &err) {
		ptr.reset();
		throw ToxTunError("Can't initialize ToxTunCore");
	} catch (std::bad_alloc &err) {
		ptr.reset();
		throw ToxTunError("Can't allocate memory");
	}

	return ptr;
}

ToxTun* ToxTun::newToxTunNoExp(Tox *tox) {
	ToxTunCore *t = nullptr;

	try {
		t = new (std::nothrow) ToxTunCore(tox);
	} catch (Error &err) {
		t = nullptr;
	}
	
	return t;
}
