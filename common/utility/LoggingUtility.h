#ifndef UTILITY_LOGGINGUTILITY_H_
#define UTILITY_LOGGINGUTILITY_H_

#include <string>
#include <utility/easylogging++.h>

using namespace std;

class LoggingUtility {
public:
	LoggingUtility();
	virtual ~LoggingUtility();

	void logStats(string module, long id, int64_t delay);
	void logDebug(string message);
};

#endif /* UTILITY_LOGGINGUTILITY_H_ */
