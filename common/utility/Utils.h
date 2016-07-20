// This file is part of OpenC2X.
//
// OpenC2X is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OpenC2X is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OpenC2X.  If not, see <http://www.gnu.org/licenses/>.
//
// Authors:
// Sven Laux <slaux@mail.uni-paderborn.de>
// Gurjashan Singh Pannu <gurjashan.pannu@ccs-labs.org>
// Stefan Schneider <stefan.schneider@ccs-labs.org>
// Jan Tiemann <janhentie@web.de>


#ifndef UTILS_H_
#define UTILS_H_

#include <string>

/**
 * Contains utility functions used by all the modules.
 *
 * @ingroup common
 */
class Utils {
public:
	Utils();
	virtual ~Utils();

	/**
	 * Converts epoch time to human readable format (HH:MM:SS.ms)
	 * @param nanoTime epoch time in ns
	 * @return human readable format string
	 */
	static std::string readableTime(int64_t nanoTime);

	static int64_t currentTime();
};

#endif /* UTILS_H_ */
