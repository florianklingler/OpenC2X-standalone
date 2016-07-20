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


#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE

#include <time.h>
#include <string>
#include "LoggingUtility.h"

using namespace std;
using namespace el;

LoggingUtility::LoggingUtility(string moduleName, int expNo) {
	mModuleName = moduleName;

	Configurations confDefault("../config/logging.conf");			//default logger for debugging
	Configurations confStatistics("../config/statistics.conf");	//statistics logger for stats

	confDefault.setRemainingToDefault();
	confDefault.setGlobally(ConfigurationType::Format, mModuleName + ", %datetime{%h:%m:%s,%g} \t %level \t %msg \t\t");
	confDefault.setGlobally(ConfigurationType::Filename, "../../logs/" + to_string(expNo) + "_log_" + mModuleName + "_" + timeString() + ".log");

	confStatistics.setRemainingToDefault();
	confStatistics.setGlobally(ConfigurationType::Format, mModuleName + " \t %msg");
	confStatistics.setGlobally(ConfigurationType::Filename, "../../logs/" + to_string(expNo) + "_stats_" + mModuleName + "_" + timeString() + ".csv");

	Loggers::reconfigureLogger("default_" + mModuleName, confDefault);
	Loggers::reconfigureLogger("statistics_" + mModuleName, confStatistics);
}

LoggingUtility::~LoggingUtility() {
}

string LoggingUtility::timeString() {
	time_t rawtime;
	struct tm * timeinfo;
	char buffer[20];

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(buffer, 20, "%F_%R", timeinfo);	//format time and save in buffer

	return buffer;
}

void LoggingUtility::logStats(string message) {
	CLOG(INFO, ("statistics_" + mModuleName).c_str()) << message;
}

void LoggingUtility::logInfo(string message) {
	CLOG(INFO, ("default_" + mModuleName).c_str()) << message;
}

void LoggingUtility::logDebug(string message) {
	CLOG(DEBUG, ("default_" + mModuleName).c_str()) << message;
}

void LoggingUtility::logError(string message){
	CLOG(ERROR, ("default_" + mModuleName).c_str()) << message;
}

void LoggingUtility::logPError(string message){
	CPLOG(ERROR, ("default_" + mModuleName).c_str()) << message;
}
