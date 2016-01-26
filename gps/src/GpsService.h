#ifndef GPSSERVICE_H_
#define GPSSERVICE_H_

#include <gps.h>
#include <utility/CommunicationSender.h>
#include <utility/LoggingUtility.h>
#include <buffers/build/gps.pb.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

struct GpsConfig {
	bool mSimulateData;

	void loadConfigXML(const string &filename) {
		boost::property_tree::ptree pt;
		read_xml(filename, pt);

		mSimulateData = pt.get("gps.SimulateData", true);
	}
};

class GpsService {
public:
	GpsService(GpsConfig &config);
	~GpsService();
	bool connectToGpsd();
	int getGpsData2(struct gps_data_t* gpsdata);
	void gpsDataToString(struct gps_data_t* gpsdata, char* output_dump);
	gpsPackage::GPS gpsDataToBuffer(struct gps_data_t* gpsdata);
	void receiveData();
	void simulateData();
	static void closeGps();
	void startStreaming();
	static void stopStreaming();

private:
	GpsConfig mConfig;
	static struct gps_data_t mGpsData;
	double mLastTime;
	CommunicationSender* mSender;

};

#endif
