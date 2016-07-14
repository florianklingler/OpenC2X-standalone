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
