#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE

#include "GpsService.h"
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <iostream>
#include <cstdlib>
#include <chrono>
#include <cmath>

using namespace std;

INITIALIZE_EASYLOGGINGPP

struct gps_data_t GpsService::mGpsData;

GpsService::GpsService(GpsConfig &config) {
	mConfig = config;
	mLastTime = NAN;
	mSender = new CommunicationSender("GpsService", "3333");
	mLogger = new LoggingUtility("GPS");
	
	//for simulation only
	mRandNumberGen = default_random_engine(0);
	mBernoulli = bernoulli_distribution(0);
	mUniform = uniform_real_distribution<double>(-0.01, 0.01);

	if (!mConfig.mSimulateData) {	//use real GPS data
		while (!connectToGpsd()) {
			// Could not connect to GPSd. Keep trying every 1 sec
			sleep(1);
		}

		startStreaming();
		receiveData();
	}
	else {				//use simulated GPS data
		position startPosition(51.732724, 8.735936);	//start position at HNI: latitude, longitude
		mTimer = new boost::asio::deadline_timer(mIoService, boost::posix_time::millisec(100));
		mTimer->async_wait(boost::bind(&GpsService::simulateData, this, boost::asio::placeholders::error, startPosition));
		mIoService.run();
	}
}

GpsService::~GpsService() {
	stopStreaming();
	closeGps();
	delete mSender;
	delete mLogger;

	mTimer->cancel();
	delete mTimer;
}


//receive & use actual GPS data with gpsd

bool GpsService::connectToGpsd() {
	if (gps_open("localhost", "2947", &mGpsData) < 0) {
		cout << "Could not connect to GPSd" << endl;
		return false;
	}
	return true;
}

//writes actual GPS in gpsdata struct
int GpsService::getGpsData(gps_data_t* gpsdata) {
	while (1) {
		if (!gps_waiting(gpsdata, 1 * 1000 * 1000)) {
			continue;
		}
		if (gps_read(gpsdata) == -1) {
			fprintf(stderr, "GPSd Error\n");
			return (-1);	//error
		}
		if (gpsdata->set && gpsdata->status > STATUS_NO_FIX) {
			break;
		}
	}
	return 0;				//success
}

//converts measured gpsdata struct to GPS protobuf
gpsPackage::GPS GpsService::gpsDataToBuffer(struct gps_data_t* gpsdata) {
	gpsPackage::GPS buffer;

	buffer.set_latitude(gpsdata->fix.latitude);
	buffer.set_longitude(gpsdata->fix.longitude);
	buffer.set_altitude(gpsdata->fix.altitude);
	buffer.set_epx(gpsdata->fix.epx);
	buffer.set_epy(gpsdata->fix.epy);
	buffer.set_time(gpsdata->fix.time);
	buffer.set_online(gpsdata->online);
	buffer.set_satellites(gpsdata->satellites_visible);

	return buffer;
}

//receives actual GPS data, logs and sends it
void GpsService::receiveData() {
	while (1) {
		if (getGpsData(&mGpsData) != 0) {				//if gpsd error, skip this iteration
			continue;
		}
		if (mGpsData.fix.time != mGpsData.fix.time) {	//??
			continue;
		}
		if (mGpsData.fix.time == mLastTime) {			//if no time progressed since last GPS, skip this iteration
			continue;
		}
		mLastTime = mGpsData.fix.time;

		gpsPackage::GPS buffer = gpsDataToBuffer(&mGpsData);
		sendToServices(buffer);
	}
}


//simulate & use fake GPS data for testing

//simulates realistic vehicle speed
double GpsService::simulateSpeed() {
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

//calculates new position: start + offsetN/E in north/east direction in meters
position GpsService::simulateNewPosition(position start, double offsetN, double offsetE) {
	 //Position, decimal degrees
	 double lat = start.first;
	 double lon = start.second;

	 //Earth’s radius, sphere
	 double R=6378137;

	 //Coordinate offsets in radians
	 double dLat = offsetN/R;
	 double dLon = offsetE/(R*cos(M_PI*lat/180));

	 //OffsetPosition, decimal degrees
	 double latO = lat + dLat * 180/M_PI;
	 double lonO = lon + dLon * 180/M_PI;

	 return make_pair(latO, lonO);
}

//simulates GPS data, logs and sends it
void GpsService::simulateData(const boost::system::error_code &ec, position currentPosition) {
	gpsPackage::GPS buffer;

	double speed = simulateSpeed();			//current speed in m/s

	//write current position to protocol buffer
	buffer.set_latitude(currentPosition.first);
	buffer.set_longitude(currentPosition.second);
	buffer.set_altitude(0);
	buffer.set_epx(0);
	buffer.set_epy(0);
	buffer.set_time(chrono::high_resolution_clock::now().time_since_epoch() / chrono::nanoseconds(1));
	buffer.set_online(0);
	buffer.set_satellites(1);

	sendToServices(buffer);
	currentPosition = simulateNewPosition(currentPosition, speed/10, 0);	//calculate new position (drive north with current speed converted to m/s)

	mTimer->expires_from_now(boost::posix_time::millisec(100));
	mTimer->async_wait(boost::bind(&GpsService::simulateData, this, boost::asio::placeholders::error, currentPosition));
}


//other

//logs and sends GPS
void GpsService::sendToServices(gpsPackage::GPS gps) {
	//log position
	string csvPosition = to_string(gps.latitude()) + "\t" + to_string(gps.longitude()) + "\t" + to_string(gps.altitude());
	mLogger->logInfo(csvPosition);

	//send buffer to services
	string serializedGps;
	gps.SerializeToString(&serializedGps);
	mSender->sendData("GPS", serializedGps);
	cout << "Sent GPS with latitude: " << gps.latitude() << ", longitude: " << gps.longitude() << endl;
}

void GpsService::closeGps() {
	gps_close(&mGpsData);
}

void GpsService::startStreaming() {
	gps_stream(&mGpsData, WATCH_ENABLE | WATCH_JSON, NULL);
}

void GpsService::stopStreaming() {
	gps_stream(&mGpsData, WATCH_DISABLE, NULL);
}

void sigHandler(int sigNum) {
	cout << "Caught signal: " << sigNum << endl;
	GpsService::stopStreaming();
	GpsService::closeGps();
}


int main() {
	GpsConfig config;
	try {
		config.loadConfigXML("../src/config.xml");
	}
	catch (std::exception &e) {
		cerr << "Error while loading config.xml: " << e.what() << endl << flush;
		return EXIT_FAILURE;
	}
	GpsService gps(config);

	signal(SIGINT, &sigHandler);
	signal(SIGTERM, &sigHandler);

	return 0;
}
