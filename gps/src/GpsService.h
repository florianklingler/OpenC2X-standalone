#ifndef GPSSERVICE_H_
#define GPSSERVICE_H_

#include <gps.h>

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
	gps_data_t* getGpsData();
	int getGpsData2(gps_data_t* gpsdata);
	void receiveData();
	void gpsDataToString(gps_data_t* gpsdata, char* output_dump);
	static void closeGps();
	void startStreaming();
	static void stopStreaming();

private:
	static gps_data_t* mGpsData;
	GPSDataContainer* mDataContainer;
	double mLastTime;
};

#endif
