#ifndef DENMAPP_H_
#define DENMAPP_H_

#include <utility/LoggingUtility.h>
#include <utility/CommunicationSender.h>
#include <buffers/build/trigger.pb.h>

class DenmApp {
public:
	DenmApp();
	~DenmApp();

	void triggerDenm(string content);

private:
	LoggingUtility* mLogger;
	CommunicationSender* mSenderToDenm;

};

#endif
