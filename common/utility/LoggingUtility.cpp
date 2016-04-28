#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE

#include <time.h>
#include <utility/LoggingUtility.h>

LoggingUtility::LoggingUtility(string moduleName) {
	mModuleName = moduleName;

	Configurations confDefault("../../logs/config/config.conf");			//default logger for debugging
	Configurations confPerformance("../../logs/config/config.conf");		//performance logger for stats

	confDefault.setRemainingToDefault();
	confDefault.setGlobally(ConfigurationType::Format, "DEBUG_" + mModuleName + " \t %datetime \t %msg");
	confDefault.setGlobally(ConfigurationType::Filename, "../../logs/debug_" + mModuleName + "_" + timeString() + ".log");

	confPerformance.setRemainingToDefault();
	confPerformance.setGlobally(ConfigurationType::Format, "STATS_" + mModuleName + " \t %msg");
	confPerformance.setGlobally(ConfigurationType::Filename, "../../logs/stats_" + mModuleName + "_" + timeString() + ".csv");

	Loggers::reconfigureLogger("default_" + mModuleName, confDefault);
	Loggers::reconfigureLogger("performance_" + mModuleName, confPerformance);
}

LoggingUtility::~LoggingUtility() {
}

string LoggingUtility::timeString() {
	time_t rawtime;
	struct tm * timeinfo;
	char buffer[15];

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(buffer, 15, "%m-%d-%y_%R", timeinfo);	//format time and save in buffer

	return buffer;
}

void LoggingUtility::logStats(string messageType, long id, int64_t delay) {
	CLOG(INFO, ("performance_" + mModuleName).c_str()) << messageType << "\t" << id << "\t" << delay;
}

void LoggingUtility::logDebug(string message) {
	CLOG(INFO, ("default_" + mModuleName).c_str()) << message;
}

void LoggingUtility::logError(string message){
	CLOG(ERROR,("default_" + mModuleName).c_str()) << "ERROR: " << message;
}

void LoggingUtility::logPError(string message){
	CPLOG(ERROR,("default_" + mModuleName).c_str()) << "ERROR: " << message;
}


