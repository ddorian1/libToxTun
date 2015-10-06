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

#ifndef ERROR_HPP
#define ERROR_HPP

/**
 * Error class for internal use.
 */
class Error {
	public:
		enum class Err {
			Permanent,
			Critical,
			Temp
		};

	private:
		const Err err;

	public:
		Error(Err err) : err(err) {};
		Err get() const {return err;};
};

#endif //ERROR_HPP
