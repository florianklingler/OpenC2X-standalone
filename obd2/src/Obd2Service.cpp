
#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE

#include "Obd2Service.h"
#include <unistd.h>
#include <iostream>
#include <cstdlib>
#include <chrono>
#include <cmath>

using namespace std;

INITIALIZE_EASYLOGGINGPP

Obd2Service::Obd2Service(Obd2Config &config) {
	mConfig = config;
	mSender = new CommunicationSender("Obd2Service", "2222");
	mLogger = new LoggingUtility("OBD2");
	
	//for simulation only
	mRandNumberGen = default_random_engine(0);
	mBernoulli = bernoulli_distribution(0);
	mUniform = uniform_real_distribution<double>(-0.01, 0.01);

	if (!mConfig.mSimulateData) {	//use real Obd2 data
		//TODO: get real data
	}
	else {				//use simulated Obd2 data
		mTimer = new boost::asio::deadline_timer(mIoService, boost::posix_time::millisec(100));
		mTimer->async_wait(boost::bind(&Obd2Service::simulateData, this, boost::asio::placeholders::error));
		mIoService.run();
	}
}

Obd2Service::~Obd2Service() {
	delete mSender;
	delete mLogger;

	mTimer->cancel();
	delete mTimer;
}


//simulates realistic vehicle speed
double Obd2Service::simulateSpeed() {
	double pCurr = mBernoulli.p();
	double pNew = pCurr + mUniform(mRandNumberGen);
	pNew = min(0.15, max(0.0, pNew));		//pNew always between 0 and 0.15

	mBernoulli = bernoulli_distribution(pNew);

	double sum = 0;

	for (int i=0; i<1000; i++) {
		double r = mBernoulli(mRandNumberGen);
		sum += min(1.0, max(0.0, r)) * 100;	//*100 to convert to 0-15 m/s
	}

	return sum / 1000.0;					//avg to avoid rapid/drastic changes in speed
}

//simulates Obd2 data, logs and sends it
void Obd2Service::simulateData(const boost::system::error_code &ec) {
	obd2Package::OBD2 obd2;

	//write current speed to protocol buffer
	obd2.set_speed(simulateSpeed());
	obd2.set_time(chrono::high_resolution_clock::now().time_since_epoch() / chrono::nanoseconds(1));

	sendToServices(obd2);

	mTimer->expires_from_now(boost::posix_time::millisec(100));
	mTimer->async_wait(boost::bind(&Obd2Service::simulateData, this, boost::asio::placeholders::error));
}

//logs and sends Obd2
void Obd2Service::sendToServices(obd2Package::OBD2 obd2) {
	//log speed
	mLogger->logDebug(to_string(obd2.speed()));

	//send buffer to services
	string serializedObd2;
	obd2.SerializeToString(&serializedObd2);
	mSender->sendData("OBD2", serializedObd2);
	cout << "sent OBD2 data with speed " << obd2.speed()*3.6 << " km/h" << endl;
}

int main() {
	Obd2Config config;
	try {
		config.loadConfigXML("../src/config.xml");
	}
	catch (std::exception &e) {
		cerr << "Error while loading config.xml: " << e.what() << endl << flush;
		return EXIT_FAILURE;
	}
	Obd2Service obd2(config);

	return 0;
}
