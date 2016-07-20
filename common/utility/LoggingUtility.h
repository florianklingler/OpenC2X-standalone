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


#ifndef UTILITY_LOGGINGUTILITY_H_
#define UTILITY_LOGGINGUTILITY_H_


#include <string>
#include "external/easylogging++.h"


/**
 * A wrapper class for the easylogging++ libary.
 *
 *
 * @ingroup common
 */
class LoggingUtility {
public:
	/**
	 * If the module name is not unique all logger with the same
	 * name will log into the same file.
	 * Experiment Number will be put into the logging file name.
	 *
	 * @todo unify all configs into a singel global conifg.
	 * Shouldn't be that hard as the individual configs use their owners name in the root xmltag.
	 *
	 * @param moduleName Module name
	 * @param expNo Experiment Number
	 */
	LoggingUtility(std::string moduleName, int expNo);
	virtual ~LoggingUtility();

	std::string timeString();
	void logStats(std::string message);
	void logInfo(std::string message);
	void logDebug(std::string message);
	void logPError(std::string message);
	void logError(std::string message);

private:
	std::string mModuleName;
};

#endif /* UTILITY_LOGGINGUTILITY_H_ */
