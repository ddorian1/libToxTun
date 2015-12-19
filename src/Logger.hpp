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

#include <iostream>
#include <sstream>

/** \file */

/**
 * Some logging helper functions
 */
namespace Logger {
	/**
	 * Concat last element to string
	 */
	template<typename T>
	static std::string concat(T val) noexcept {
		std::ostringstream s;
		s << val;
		return s.str();
	}

	/**
	 * Concat to string
	 */
	template<typename First, typename ... Rest>
	std::string concat(First first, Rest ... rest) noexcept {
		std::ostringstream s;
		s << first << concat(rest ...);
		return s.str();
	}

	/**
	 * Log a debug message
	 */
	template<typename ... Args>
	void debug(Args ... args) noexcept {
#ifdef DEBUG
		std::cout << "DEBUG:\t" << concat(args ...) << std::endl;
#endif
	}

	/**
	 * Log an error message
	 */
	template<typename ... Args>
	void error(Args ... args) noexcept {
		std::cout << "ERROR:\t" << concat(args ...) << std::endl;
	}
} //namespace Logger
