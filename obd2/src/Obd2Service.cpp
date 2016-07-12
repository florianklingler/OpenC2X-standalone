
#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE

#include "Obd2Service.h"
#include <unistd.h>
#include <iostream>
#include <cstdlib>
#include <chrono>
#include <cmath>
#include <utility/Utils.h>

using namespace std;

INITIALIZE_EASYLOGGINGPP

Obd2Service::Obd2Service(Obd2Config &config) {
	try {
		mGlobalConfig.loadConfigXML("../../common/config/config.xml");
	}
	catch (std::exception &e) {
		cerr << "Error while loading config.xml: " << e.what() << endl;
	}
	mConfig = config;
	mSender = new CommunicationSender("Obd2Service", "2222", mGlobalConfig.mExpNo);
	mLogger = new LoggingUtility("Obd2Service", mGlobalConfig.mExpNo);
	mLogger->logStats("speed (m/sec)");
	
	//for simulation only
	mRandNumberGen = default_random_engine(0);
	mBernoulli = bernoulli_distribution(0);
	mUniform = uniform_real_distribution<double>(-0.01, 0.01);

}

Obd2Service::~Obd2Service() {
	delete mSender;
	delete mLogger;

	mTimer->cancel();
	delete mTimer;
}

//reads the actual vehicle data from OBD2
void Obd2Service::receiveData(const boost::system::error_code &ec, SerialPort* serial) {
	double speed = serial->readSpeed();
	int rpm = serial->readRpm();

	if (speed != -1) {		//valid speed
		//write current data to protocol buffer
		obd2Package::OBD2 obd2;
		obd2.set_time(Utils::currentTime());
		obd2.set_speed(speed);
		if (rpm != -1) {
			obd2.set_rpm(rpm);
		}

		sendToServices(obd2);
	}

	mTimer->expires_from_now(boost::posix_time::millisec(mConfig.mFrequency));
	mTimer->async_wait(boost::bind(&Obd2Service::receiveData, this, boost::asio::placeholders::error, serial));
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
	obd2.set_time(Utils::currentTime());

	sendToServices(obd2);

	mTimer->expires_from_now(boost::posix_time::millisec(mConfig.mFrequency));
	mTimer->async_wait(boost::bind(&Obd2Service::simulateData, this, boost::asio::placeholders::error));
}

//logs and sends Obd2
void Obd2Service::sendToServices(obd2Package::OBD2 obd2) {
	//send buffer to services
	string serializedObd2;
	obd2.SerializeToString(&serializedObd2);
	mSender->sendData("OBD2", serializedObd2);
	mLogger->logStats(to_string(obd2.speed())); // In csv, we log speed in m/sec
}

void Obd2Service::init() {
	if (!mConfig.mSimulateData) {	//use real Obd2 data
		SerialPort* serial = new SerialPort();
		if (serial->connect(mConfig.mDevice) != -1) {
			mLogger->logInfo("Connected to serial port successfully");

			serial->init();

			mTimer = new boost::asio::deadline_timer(mIoService, boost::posix_time::millisec(mConfig.mFrequency));
			mTimer->async_wait(boost::bind(&Obd2Service::receiveData, this, boost::asio::placeholders::error, serial));
			mIoService.run();

			serial->disconnect();
		}
		else {
			mLogger->logError("Cannot open serial port -> plug in OBD2 and run with sudo");
		}
	}
	else {				//use simulated Obd2 data
		mTimer = new boost::asio::deadline_timer(mIoService, boost::posix_time::millisec(mConfig.mFrequency));
		mTimer->async_wait(boost::bind(&Obd2Service::simulateData, this, boost::asio::placeholders::error));
		mIoService.run();
	}
}

int main() {
	Obd2Config config;
	try {
		config.loadConfigXML("../config/config.xml");
	}
	catch (std::exception &e) {
		cerr << "Error while loading config.xml: " << e.what() << endl << flush;
		return EXIT_FAILURE;
	}
	Obd2Service obd2(config);
	obd2.init();

	return 0;
}
