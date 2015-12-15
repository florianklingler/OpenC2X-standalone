#define ELPP_NO_DEFAULT_LOG_FILE

#include <utility/LoggingUtility.h>

using namespace el;


LoggingUtility::LoggingUtility() {
	Configurations confDefault;
	Configurations confPerformance;

	confDefault.setToDefault();
	confDefault.setGlobally(ConfigurationType::Filename, "../../logs/debug.log");
	confDefault.setGlobally(ConfigurationType::ToStandardOutput, "false");

	confPerformance.setRemainingToDefault();
	confPerformance.setGlobally(ConfigurationType::Format, "%datetime, %msg");
	confPerformance.setGlobally(ConfigurationType::Filename, "../../logs/stats.csv");
	confPerformance.setGlobally(ConfigurationType::ToStandardOutput, "false");

	Loggers::reconfigureLogger("default", confDefault);
	Loggers::reconfigureLogger("performance", confPerformance);
}

LoggingUtility::~LoggingUtility() {
}

//TODO: Thread safe?
void LoggingUtility::logStats(string module, string event) {
	Logger* performanceLogger = Loggers::getLogger("performance");

	performanceLogger->info(module + ", " + event);
	logDebug(module + ", " + event);
}

void LoggingUtility::logDebug(string message) {
	Logger* defaultLogger = Loggers::getLogger("default");

	defaultLogger->info(message);
}
