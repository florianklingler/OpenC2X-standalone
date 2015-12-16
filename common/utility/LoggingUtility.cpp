#define ELPP_NO_DEFAULT_LOG_FILE

#include <utility/LoggingUtility.h>

using namespace el;


LoggingUtility::LoggingUtility() {
	Configurations confDefault;			//default logger for debugging
	Configurations confPerformance;		//performance logger for stats

	confDefault.setToDefault();
	confDefault.setGlobally(ConfigurationType::Filename, "../../logs/debug.log");
	confDefault.setGlobally(ConfigurationType::ToStandardOutput, "false");

	confPerformance.setRemainingToDefault();
	confPerformance.setGlobally(ConfigurationType::Format, "%msg");
	confPerformance.setGlobally(ConfigurationType::Filename, "../../logs/stats.csv");
	confPerformance.setGlobally(ConfigurationType::ToStandardOutput, "false");

	Loggers::reconfigureLogger("default", confDefault);
	Loggers::reconfigureLogger("performance", confPerformance);
}

LoggingUtility::~LoggingUtility() {
}

//TODO: Thread safe?
void LoggingUtility::logStats(string module, long id, int64_t delay) {
	Logger* performanceLogger = Loggers::getLogger("performance");

	performanceLogger->info(module + "\t" + to_string(id) + "\t" + to_string(delay));
	logDebug(module + ", " + to_string(id) + ", " + to_string(delay));
}

void LoggingUtility::logDebug(string message) {
	Logger* defaultLogger = Loggers::getLogger("default");

	defaultLogger->info(message);
}
