#ifndef UTILITY_LOGGINGUTILITY_H_
#define UTILITY_LOGGINGUTILITY_H_

#include <string>
#include <utility/easylogging++.h>

using namespace std;
using namespace el;

class LoggingUtility {
public:
	LoggingUtility(string moduleName);
	virtual ~LoggingUtility();

	string timeString();
	void logStats(string messageType, long id, int64_t delay);
	void logDebug(string message);

private:
	string mModuleName;
};

#endif /* UTILITY_LOGGINGUTILITY_H_ */
