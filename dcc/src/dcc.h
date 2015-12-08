#include <string>
#include <boost/thread.hpp>
#include <utility/Communication.h>
#include <utility/CommunicationSender.h>
#include <utility/CommunicationReceiver.h>
#include <utility/ICommunication.h>

class DCC : public ICommunication{
public:
	DCC ();
	~DCC ();
	
	void init();
	void receiveLoopFromCa();
	void receiveLoopFromDen();
    virtual string process(string message);

private:
    CommunicationReceiver* mReceiverFromDen;
	CommunicationReceiver* mReceiverFromCa;
	CommunicationSender* mSenderToLower;

	Communication* mCommunicationLowerToUpper;	//hw to CA service/DEN service

	boost::thread* receiveFromCaThread;
	boost::thread* receiveFromDenThread;
	boost::thread* receiveFromLowerThread;
};
