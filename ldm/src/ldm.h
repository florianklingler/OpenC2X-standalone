#ifndef LDM_H_
#define LDM_H_

#include <string>
#include <boost/thread.hpp>
#include <utility/Communication.h>
#include <utility/ICommunication.h>
#include <utility/CommunicationReceiver.h>

class LDM : public ICommunication {
public:
	LDM();
	~LDM();
	void init();

  	void receiveLoopFromCa();
  	void receiveLoopFromDen();
  	virtual string process(string message);
  	
private:
  	CommunicationReceiver* mReceiverFromDen;
  	CommunicationReceiver* mReceiverFromCa;
	
	boost::thread* mReceiveFromCaThread;
	boost::thread* mReceiveFromDenThread;
};

#endif
