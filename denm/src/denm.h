#include <string>
#include <boost/thread.hpp>
#include <utility/Communication.h>
#include <utility/ICommunication.h>

using namespace std;

class DENM : public ICommunication{
public:
	DENM();
	~DENM();
	void init();
	void sendTestLoop();
    virtual string process(string message);

	
private:
	Communication* mCommunicationDccToLdm;
	CommunicationSender* mSenderDcc;
	
	boost::thread* mToDccThread;
	boost::thread* mDccToLdmThread;
};
