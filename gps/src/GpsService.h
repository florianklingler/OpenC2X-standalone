#ifndef GPSSERVICE_H_
#define GPSSERVICE_H_

#include <gps.h>
#include <utility/CommunicationSender.h>
#include <utility/LoggingUtility.h>
#include <buffers/build/gps.pb.h>

struct GPSDataContainer {
	double mLat;
	double mLong;
	double mAlt;
};

class GpsService {
public:
	GpsService(bool simulate);
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
	static struct gps_data_t mGpsData;
	//GPSDataContainer* mDataContainer;
	double mLastTime;
	CommunicationSender* mSender;

};

#endif
