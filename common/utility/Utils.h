/*
 * Utils.h
 *
 *  Created on: Jul 12, 2016
 *      Author: pannu
 */

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
};

#endif /* UTILS_H_ */
