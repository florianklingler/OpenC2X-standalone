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
#include <common/config/config.h>
#include "LoggingUtility.h"

using namespace std;
using namespace el;
using boost::property_tree::ptree;


LoggingUtility::LoggingUtility(string configName, string moduleName, string logBasePath, string expName, int expNo, ptree& config): config(config) {
	mModuleName = moduleName;
	
	string logFileBasePath = get_openc2x_path(logBasePath, expName, expNo);
	
	string timeStr = timeString();
	
	string statisticsPath = logFileBasePath + to_string(expNo) + "_stats_" + mModuleName + "_" + timeStr + ".csv";
	string logsPath = logFileBasePath + to_string(expNo) + "_log_" + mModuleName + "_" + timeStr + ".log";
	std::cout << statisticsPath << std::endl;
	
	string enable_statistics = config.get(configName + ".enable_statistics", "false");
	string statistics_to_file = config.get(configName + ".statistics_to_file", "false");
	string statistics_to_standard_output = config.get(configName + ".statistics_to_standard_output", "false");
	cout << "ENABLE_STATISTICS: " << enable_statistics << endl;
	Configurations confStatistics;			//statistics logger for stats
	confStatistics.setGlobally(ConfigurationType::Enabled, enable_statistics);
	confStatistics.setGlobally(ConfigurationType::ToFile, statistics_to_file);
	confStatistics.setGlobally(ConfigurationType::ToStandardOutput, statistics_to_standard_output);
	confStatistics.setRemainingToDefault();
	confStatistics.setGlobally(ConfigurationType::Format, mModuleName + " \t %msg");
	confStatistics.setGlobally(ConfigurationType::Filename, statisticsPath);
	
	
	Configurations confDefault;			//default logger for debugging

	
	//Global
	
	string enable_logging = config.get(configName + ".enable_logging", "false");
	string logging_to_file = config.get(configName + ".logging_to_file", "false");
	string logging_to_standard_output = config.get(configName + ".logging_to_standard_output", "false");

	confDefault.setGlobally(ConfigurationType::Enabled, enable_logging);
	confDefault.setGlobally(ConfigurationType::ToFile, logging_to_file);
	confDefault.setGlobally(ConfigurationType::ToStandardOutput, logging_to_standard_output);

	//Error
		
	string enable_logging_error = config.get(configName + ".enable_logging_error", "false");
	string logging_error_to_file = config.get(configName + ".logging_error_to_file", "false");
	string logging_error_to_standard_output = config.get(configName + ".logging_error_to_standard_output", "false");
	
	confDefault.set(Level::Error, ConfigurationType::Enabled, enable_logging_error);
	confDefault.set(Level::Error, ConfigurationType::ToFile, logging_error_to_file);
	confDefault.set(Level::Error, ConfigurationType::ToStandardOutput, logging_error_to_standard_output);

	//Info
	
	string enable_logging_info = config.get(configName + ".enable_logging_info", "false");
	string logging_info_to_file = config.get(configName + ".logging_info_to_file", "false");
	string logging_info_to_standard_output = config.get(configName + ".logging_info_to_standard_output", "false");
	
	confDefault.set(Level::Info, ConfigurationType::Enabled, enable_logging_info);
	confDefault.set(Level::Info, ConfigurationType::ToFile, logging_info_to_file);
	confDefault.set(Level::Info, ConfigurationType::ToStandardOutput, logging_info_to_standard_output);
	
	//Debug
		
	string enable_logging_debug = config.get(configName + ".enable_logging_debug", "false");
	string logging_debug_to_file = config.get(configName + ".logging_debug_to_file", "false");
	string logging_debug_to_standard_output = config.get(configName + ".logging_debug_to_standard_output", "false");
	
	confDefault.set(Level::Debug, ConfigurationType::Enabled, enable_logging_debug);
	confDefault.set(Level::Debug, ConfigurationType::ToFile, logging_debug_to_file);
	confDefault.set(Level::Debug, ConfigurationType::ToStandardOutput, logging_debug_to_standard_output);
	
	
	confDefault.setRemainingToDefault();
	confDefault.setGlobally(ConfigurationType::Format, mModuleName + ", %datetime{%h:%m:%s,%g} \t %level \t %msg \t\t");
	confDefault.setGlobally(ConfigurationType::Filename, logsPath);

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
