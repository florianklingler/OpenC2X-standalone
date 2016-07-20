#ifndef DENMAPP_H_
#define DENMAPP_H_

#include <string>
#include <stdio.h>
#include <unistd.h>
#include <utility/CommunicationSender.h>
#include <buffers/build/trigger.pb.h>

/*
 * This is a simple application that can trigger DENMs using a command-line interface.
 * It is not used in the current stack. If you want to use it, compile and
 * run "./denmApp <content>" from within denmApp/Debug.
 */
class DenmApp {
public:
	DenmApp();
	~DenmApp();

	/*
	 * Sends a request to trigger a DENM with the specified content to the DenService.
	 * @param content The content of the DENM to be triggered.
	 */
	void triggerDenm(std::string content);

private:
	CommunicationSender* mSenderToDenm;
};

#endif
