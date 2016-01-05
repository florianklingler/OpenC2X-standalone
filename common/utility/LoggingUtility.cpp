#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE

#include <time.h>
#include <utility/LoggingUtility.h>

LoggingUtility::LoggingUtility(string moduleName) {
	mModuleName = moduleName;

	Configurations confDefault;			//default logger for debugging
	Configurations confPerformance;		//performance logger for stats

	confDefault.setToDefault();
	confDefault.setGlobally(ConfigurationType::Format, "%level \t %datetime \t %msg");
	confDefault.setGlobally(ConfigurationType::ToStandardOutput, "true");

	confPerformance.setRemainingToDefault();
	confPerformance.setGlobally(ConfigurationType::Format, "STATS \t %msg");
	confPerformance.setGlobally(ConfigurationType::ToStandardOutput, "true");

	Loggers::reconfigureLogger("default_" + mModuleName, confDefault);
	Loggers::reconfigureLogger("performance_" + mModuleName, confPerformance);
}

LoggingUtility::~LoggingUtility() {
}

void LoggingUtility::logStats(string messageType, long id, int64_t delay) {
	CLOG(INFO, ("performance_" + mModuleName).c_str()) << messageType + "\t" + to_string(id) + "\t" + to_string(delay);
}

void LoggingUtility::logDebug(string message) {
	CLOG(INFO, ("default_" + mModuleName).c_str()) << message;
}
