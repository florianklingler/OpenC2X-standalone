#ifndef GPSSERVICE_H_
#define GPSSERVICE_H_

#include <gps.h>
#include <utility/CommunicationSender.h>
#include <utility/LoggingUtility.h>
#include <buffers/build/gps.pb.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/asio.hpp>

struct GpsConfig {
	bool mSimulateData;

	void loadConfigXML(const string &filename) {
		boost::property_tree::ptree pt;
		read_xml(filename, pt);

		mSimulateData = pt.get("gps.SimulateData", true);
	}
};

typedef struct pair<double, double> position;

class GpsService {
public:
	GpsService(GpsConfig &config);
	~GpsService();

	bool connectToGpsd();
	int getGpsData(struct gps_data_t* gpsdata);
	gpsPackage::GPS gpsDataToBuffer(struct gps_data_t* gpsdata);
	void receiveData();

	double simulateSpeed();
	void simulateData(const boost::system::error_code &ec, position currentPosition);
	position simulateNewPosition(position start, double offsetN, double offsetE);

	void sendToServices(gpsPackage::GPS buffer);
	static void closeGps();
	void startStreaming();
	static void stopStreaming();

private:
	GpsConfig mConfig;
	static struct gps_data_t mGpsData;
	double mLastTime;		//time of last received/measured GPS

	CommunicationSender* mSender;
	LoggingUtility* mLogger;

	//for simulation only
	default_random_engine mRandNumberGen;
	bernoulli_distribution mBernoulli;
	uniform_real_distribution<double> mUniform;

	boost::asio::io_service mIoService;
	boost::asio::deadline_timer* mTimer;
};

#endif
