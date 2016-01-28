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
	delete mSender;
}

bool GpsService::connectToGpsd() {
	if (gps_open("localhost", "2947", &mGpsData) < 0) {
		cout << "Could not connect to GPSd" << endl;
		return false;
	}
	return true;
}

int GpsService::getGpsData2(gps_data_t* gpsdata) {
	while (1) {
		if (!gps_waiting(gpsdata, 1 * 1000 * 1000)) {
			continue;
		}
		if (gps_read(gpsdata) == -1) {
			fprintf(stderr, "GPSd Error\n");
			return (-1);
		}
		if (gpsdata->set && gpsdata->status > STATUS_NO_FIX) {
			break;
		}
	}
	return 0;
}


void GpsService::gpsDataToString(struct gps_data_t* gpsdata, char* output_dump) {
	sprintf(output_dump,
			"%17.12f,%17.12f,%17.12f,%17.12f,%23.12f,%23.12f,%3u\n",
			gpsdata->fix.latitude, gpsdata->fix.longitude,
			gpsdata->fix.altitude,
			(gpsdata->fix.epx > gpsdata->fix.epy) ?
					gpsdata->fix.epx : gpsdata->fix.epy, gpsdata->fix.time,
			gpsdata->online, gpsdata->satellites_visible);
}

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

void GpsService::receiveData() {
	char gpsDumpMsg[1024];
	sprintf(gpsDumpMsg, "%17s,%17s,%17s,%17s,%23s,%23s,%3s\n", "Lat", "Lon",
			"Alt", "Accuracy", "Time", "Online", "Sat");
	while (1) {
		if (getGpsData2(&mGpsData) != 0) {
			continue;
		}
		if (mGpsData.fix.time != mGpsData.fix.time) {
			continue;
		}
		if (mGpsData.fix.time == mLastTime) {
			continue;
		}
		mLastTime = mGpsData.fix.time;

//		gpsDataToString(&mGpsData, gpsDumpMsg);
//		fprintf(stdout, "%s", gpsDumpMsg);
		gpsPackage::GPS buffer = gpsDataToBuffer(&mGpsData);
		string serializedGps;
		buffer.SerializeToString(&serializedGps);
		mSender->sendGpsData("GPS", serializedGps);
	}
}

//simulates realistic vehicle speed; to be replaced with actual measurements
double GpsService::simulateSpeed() {		//just for testing/simulation
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

		//send buffer to CaService
		buffer.SerializeToString(&serializedGps);
		mSender->sendGpsData("GPS", serializedGps);
		//TODO: send to DEN
		cout << "sent GPS data" << endl;

		//calculate new position
		sleep(1);	//1s	//TODO: use deadline_timer?
		position = simulateNewPosition(position, (speed/3.6), 0);	//drive north with current speed converted to m/s
	}
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
