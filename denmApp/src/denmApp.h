#ifndef DENMAPP_H_
#define DENMAPP_H_

#include <string>
#include <stdio.h>
#include <unistd.h>
#include <utility/CommunicationSender.h>
#include <buffers/build/trigger.pb.h>

class DenmApp {
public:
	DenmApp();
	~DenmApp();

	void triggerDenm(std::string content);

private:
	CommunicationSender* mSenderToDenm;
};

#endif
