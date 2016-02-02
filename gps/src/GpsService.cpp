#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE

#include "GpsService.h"
#include <unistd.h>
#include <math.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <iostream>
#include <cstdlib>
#include <chrono>
#include <math.h>

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
	mUniform = uniform_real_distribution<double>(-0.1, 0.1);

	if (!mConfig.mSimulateData) {	//use real GPS data
		while (!connectToGpsd()) {
			// Could not connect to GPSd. Keep trying every 1 sec
			sleep(1);
		}

		startStreaming();
		receiveData();
	}
	else {				//use simulated GPS data
		simulateData();
	}
}

GpsService::~GpsService() {
	stopStreaming();
	closeGps();
	delete mSender;
	delete mLogger;
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
	pNew = min(0.5, max(0.0, pNew));		//pNew always between 0 and 0.5 -> 0-50km/h

	mBernoulli = bernoulli_distribution(pNew);

	double sum = 0;

	for (int i=0; i<1000; i++) {
		double r = mBernoulli(mRandNumberGen);
		sum += min(1.0, max(0.0, r)) * 100;	//*100 to convert to km/h
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
void GpsService::simulateData() {
	string serializedGps;
	gpsPackage::GPS buffer;

	position position(51.732724, 8.735936);	//start position at HNI: latitude, longitude
	double speed; 							//current speed in kmh

	while (1) {
		speed = simulateSpeed();
		cout << "current speed: " << speed << endl;

		//write current position to protocol buffer
		buffer.set_latitude(position.first);	//TODO: why only 4 digits accuracy?
		buffer.set_longitude(position.second);
		buffer.set_altitude(0);
		buffer.set_epx(0);
		buffer.set_epy(0);
		buffer.set_time(chrono::high_resolution_clock::now().time_since_epoch() / chrono::nanoseconds(1));
		buffer.set_online(0);
		buffer.set_satellites(1);

		sendToServices(buffer);
		position = simulateNewPosition(position, (speed/3.6), 0);	//calculate new position (drive north with current speed converted to m/s)

		sleep(1);	//1s	//TODO: use deadline_timer?
	}
}


//other

//logs and sends GPS
void GpsService::sendToServices(gpsPackage::GPS buffer) {
	//log position
	string csvPosition = to_string(buffer.latitude()) + "\t" + to_string(buffer.longitude()) + "\t" + to_string(buffer.altitude());
	mLogger->logDebug(csvPosition);

	//send buffer to CaService
	string serializedGps;
	buffer.SerializeToString(&serializedGps);
	mSender->sendGpsData("GPS", serializedGps);
	//TODO: send to DEN
	cout << "sent GPS data" << endl;
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
