#include <string>
#include <boost/thread.hpp>
#include <utility/Communication.h>
#include <utility/ICommunication.h>

using namespace std;

class CAM: public ICommunication {
public:
	CAM();
	~CAM();

	void init();
	void sendTestLoop();
	virtual string process(string message);

private:
	Communication* mCommunicationDccToLdm;
	CommunicationSender* mSenderDcc;

	boost::thread* mToDccThread;
	boost::thread* mDccToLdmThread;
};
