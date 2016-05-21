#ifndef DENMAPP_H_
#define DENMAPP_H_

#include <utility/LoggingUtility.h>

class DenmApp {
public:
	DenmApp();
	~DenmApp();

	void init();


private:
	LoggingUtility* mLogger;

};

#endif
