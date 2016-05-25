#ifndef LDM_H_
#define LDM_H_

#include <boost/thread.hpp>
#include <utility/CommunicationReceiver.h>
#include <utility/LoggingUtility.h>
#include <sqlite3.h>
#include <buffers/build/cam.pb.h>
#include <buffers/build/denm.pb.h>
#include <buffers/build/gps.pb.h>
#include <google/protobuf/text_format.h>

class LDM {
public:
	LDM();
	~LDM();
	void init();

	void receiveFromCa();
	void insertCam(camPackage::CAM cam);
	void receiveFromDen();

private:
	CommunicationReceiver* mReceiverFromDen;
	CommunicationReceiver* mReceiverFromCa;

	boost::thread* mThreadReceiveFromCa;
	boost::thread* mThreadReceiveFromDen;

	LoggingUtility* mLogger;

	sqlite3 *mDb;
};

#endif
