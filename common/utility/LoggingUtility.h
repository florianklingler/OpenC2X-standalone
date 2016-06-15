#ifndef UTILITY_LOGGINGUTILITY_H_
#define UTILITY_LOGGINGUTILITY_H_

#include <string>
#include <utility/easylogging++.h>

class LoggingUtility {
public:
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
