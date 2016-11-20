// This file is part of OpenC2X.
//
// OpenC2X is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OpenC2X is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OpenC2X.  If not, see <http://www.gnu.org/licenses/>.
//
// Authors:
// Sven Laux <slaux@mail.uni-paderborn.de>
// Gurjashan Singh Pannu <gurjashan.pannu@ccs-labs.org>
// Stefan Schneider <stefan.schneider@ccs-labs.org>
// Jan Tiemann <janhentie@web.de>
// Christoph Sommer <sommer@ccs-labs.org>

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
#include <string>
#include <utility/Utils.h>
#include <boost/algorithm/string.hpp>


using namespace std;

INITIALIZE_EASYLOGGINGPP

struct gps_data_t GpsService::mGpsData;

GpsService::GpsService(GpsConfig &config) {
	try {
		mGlobalConfig.loadConfigXML("../../common/config/config.xml");
	}
	catch (std::exception &e) {
		cerr << "Error while loading config.xml: " << e.what() << endl;
	}
	mConfig = config;
	mLastTime = NAN;
	mSender = new CommunicationSender("GPS", "3333", mGlobalConfig.mExpNo);
	mLogger = new LoggingUtility("GPS", mGlobalConfig.mExpNo);
	
	//for simulation only
	mRandNumberGen = default_random_engine(0);
	mBernoulli = bernoulli_distribution(0);
	mUniform = uniform_real_distribution<double>(-0.01, 0.01);

}

GpsService::~GpsService() {
	stopStreaming();
	closeGps();
	delete mSender;
	delete mLogger;

	if(mConfig.mSimulateData) {
		if(mConfig.mMode == 0) {
			mTimer->cancel();
			delete mTimer;
		} else if(mConfig.mMode == 1) {
			mFile.close();
		}
	}
}


//receive & use actual GPS data with gpsd

bool GpsService::connectToGpsd() {
	if (gps_open("localhost", "2947", &mGpsData) < 0) {
		mLogger->logError("Could not connect to GPSd");
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
			mLogger->logError("GPSd Error");
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
	if(gpsdata->fix.epx != gpsdata->fix.epx) { // set epx to -1 in case of NaN
		buffer.set_epx(-1);
	} else {
		buffer.set_epx(gpsdata->fix.epx);
	}
	if(gpsdata->fix.epy != gpsdata->fix.epy) { // set epy to -1 in case of NaN
		buffer.set_epy(-1);
	} else {
		buffer.set_epy(gpsdata->fix.epy);
	}
//	buffer.set_time(gpsdata->fix.time);	//FIXME: convert GPS time to epoch time
//	buffer.set_online(gpsdata->online);
	buffer.set_time(Utils::currentTime());
	buffer.set_online(0);
	if(gpsdata->satellites_visible != gpsdata->satellites_visible) { // set satellites visible to -1 in case of NaN
		buffer.set_satellites(-1);
	} else {
		buffer.set_satellites(gpsdata->satellites_visible);
	}

	return buffer;
}

//receives actual GPS data, logs and sends it
void GpsService::receiveData() {
	while (1) {
		if (getGpsData(&mGpsData) != 0) {				//if gpsd error, skip this iteration
			continue;
		}
		if (mGpsData.fix.time != mGpsData.fix.time) {	//skip if time is NaN
			continue;
		}
		if (mGpsData.fix.latitude != mGpsData.fix.latitude || mGpsData.fix.longitude != mGpsData.fix.longitude || mGpsData.fix.altitude != mGpsData.fix.altitude) {	//skip if invalid position (NaN)
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
	buffer.set_time(Utils::currentTime());
	buffer.set_online(0);
	buffer.set_satellites(1);

	sendToServices(buffer);
	currentPosition = simulateNewPosition(currentPosition, speed/10, 0);	//calculate new position (drive north with current speed converted to m/s)

	mTimer->expires_from_now(boost::posix_time::millisec(100));
	mTimer->async_wait(boost::bind(&GpsService::simulateData, this, boost::asio::placeholders::error, currentPosition));
}

void GpsService::simulateFromDemoTrail(const boost::system::error_code &ec) {
	string line;
	while(1) {
		while(getline(mFile, line)) {
			gpsPackage::GPS buffer = convertTrailDataToBuffer(line);
			sendToServices(buffer);
			usleep(500000);
		}
		mFile.clear();
		mFile.seekg(0, ios::beg);
	}
}

gpsPackage::GPS GpsService::convertTrailDataToBuffer(string line) {
	gpsPackage::GPS buffer;
	vector<string> values;
	boost::split(values, line, boost::is_any_of("\t"));
	buffer.set_latitude(stod(values[1]));
	buffer.set_longitude(stod(values[2]));
	buffer.set_altitude(stod(values[3]));
	buffer.set_epx(0);
	buffer.set_epy(0);
	buffer.set_time(Utils::currentTime());
	buffer.set_online(0);
	buffer.set_satellites(1);
	return buffer;
}

//other

//logs and sends GPS
void GpsService::sendToServices(gpsPackage::GPS gps) {
	//send buffer to services
	string serializedGps;
	gps.SerializeToString(&serializedGps);
	mSender->sendData("GPS", serializedGps);
	//log position
	string csvPosition = to_string(gps.latitude()) + "\t" + to_string(gps.longitude()) + "\t" + to_string(gps.altitude());
	mLogger->logStats(csvPosition);
}

void GpsService::init() {
	if (!mConfig.mSimulateData) {	//use real GPS data
		while (!connectToGpsd()) {
			// Could not connect to GPSd. Keep trying every 1 sec
			sleep(1);
		}
		startStreaming();
		receiveData();
	}
	else {				//use simulated GPS data
		if(mConfig.mMode == 0) {
			position startPosition(51.732724, 8.735936);	//start position at HNI: latitude, longitude
			mTimer = new boost::asio::deadline_timer(mIoService, boost::posix_time::millisec(100));
			mTimer->async_wait(boost::bind(&GpsService::simulateData, this, boost::asio::placeholders::error, startPosition));

		} else if (mConfig.mMode == 1) {
			stringstream ss;
			ss << "../gpsdata/" << mConfig.mGpsDataFile;
			mFile.open(ss.str(), fstream::in);
			if(!mFile.is_open()) {
				mLogger->logError("Failed to open gpsdata file");
				exit(1);
			}
			mTimer = new boost::asio::deadline_timer(mIoService, boost::posix_time::millisec(500));
			mTimer->async_wait(boost::bind(&GpsService::simulateFromDemoTrail, this, boost::asio::placeholders::error));
		}
		mIoService.run();
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
		config.loadConfigXML("../config/config.xml");
	}
	catch (std::exception &e) {
		cerr << "Error while loading config.xml: " << e.what() << endl << flush;
		return EXIT_FAILURE;
	}
	GpsService gps(config);
	gps.init();

	signal(SIGINT, &sigHandler);
	signal(SIGTERM, &sigHandler);

	return 0;
}
