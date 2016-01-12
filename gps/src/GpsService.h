#ifndef GPSSERVICE_H_
#define GPSSERVICE_H_

#include <gps.h>
#include <utility/GpsDataSender.h>
#include <utility/LoggingUtility.h>

struct GPSDataContainer {
	double mLat;
	double mLong;
	double mAlt;
};

class GpsService {
public:
	GpsService();
	~GpsService();
	bool connectToGpsd();
	int getGpsData2(struct gps_data_t* gpsdata);
	void receiveData();
	void gpsDataToString(struct gps_data_t* gpsdata, char* output_dump);
	static void closeGps();
	void startStreaming();
	static void stopStreaming();

private:
	static struct gps_data_t mGpsData;
	//GPSDataContainer* mDataContainer;
	double mLastTime;
	GpsDataSender* mSender;

};

#endif
