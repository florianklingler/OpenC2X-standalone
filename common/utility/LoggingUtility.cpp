#define ELPP_NO_DEFAULT_LOG_FILE

#include <time.h>
#include <utility/LoggingUtility.h>

using namespace el;


LoggingUtility::LoggingUtility(string moduleName) {
	mModuleName = moduleName;

	Configurations confDefault;			//default logger for debugging
	Configurations confPerformance;		//performance logger for stats

	confDefault.setToDefault();
	confDefault.setGlobally(ConfigurationType::Filename, "../../logs/debug_" + mModuleName + "_" + timeString() + ".log");
	confDefault.setGlobally(ConfigurationType::ToStandardOutput, "false");

	confPerformance.setRemainingToDefault();
	confPerformance.setGlobally(ConfigurationType::Format, "%msg");
	confPerformance.setGlobally(ConfigurationType::Filename, "../../logs/stats_" + mModuleName + "_" + timeString() + ".csv");
	confPerformance.setGlobally(ConfigurationType::ToStandardOutput, "false");

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

//TODO: Thread safe?
void LoggingUtility::logStats(string messageType, long id, int64_t delay) {
	Logger* performanceLogger = Loggers::getLogger("performance_" + mModuleName);

	performanceLogger->info(messageType + "\t" + to_string(id) + "\t" + to_string(delay));
	logDebug(messageType + ", " + to_string(id) + ", " + to_string(delay));
}

void LoggingUtility::logDebug(string message) {
	Logger* defaultLogger = Loggers::getLogger("default_" + mModuleName);

	defaultLogger->info(message);
}
