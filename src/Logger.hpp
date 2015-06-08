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

/** \file */

/**
 * Some logging helper functions
 */
namespace Logger {
	/**
	 * print last value and newline
	 */
	template<typename T>
	static void print(T val) {
		std::cout << val << "\n";
	}

	/**
	 * print list of values
	 */
	template<typename First, typename ... Rest>
	static void print(First first, Rest ... rest) {
		std::cout << first;
		print(rest ...);
	}

	/**
	 * log a debug message
	 */
	template<typename ... Args>
	void debug(Args ... args) {
#ifdef DEBUG
		std::cout << "DEBUG:\t";
		print(args ...);
#endif
	}

	/**
	 * log an error message
	 */
	template<typename ... Args>
	void error(Args ... args) {
		std::cout << "ERROR:\t";
		print(args ...);
	}
} //namespace Logger
