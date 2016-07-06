#ifndef CASERVICE_H_
#define CASERVICE_H_

/**
 * @addtogroup cam
 * @{
 */

#include <boost/thread.hpp>
#include <config/config.h>
#include <utility/CommunicationReceiver.h>
#include <utility/CommunicationSender.h>
#include <buffers/build/data.pb.h>
#include <buffers/build/cam.pb.h>
#include <buffers/build/gps.pb.h>
#include <buffers/build/obd2.pb.h>
#include <buffers/build/camInfo.pb.h>
#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <mutex>

/** Struct that hold the configuration for CaService.
 * The configuration is defined in <a href="../../cam/config/config.xml">cam/config/config.xml</a>
 */
struct CaServiceConfig {
	bool mGenerateMsgs;
	int mExpirationTime;
	int mMaxGpsAge;
	int mMaxObd2Age;

	void loadConfigXML(const std::string &filename) {
		boost::property_tree::ptree pt;
		read_xml(filename, pt);

		mGenerateMsgs = pt.get("cam.generateMsgs", true);
		mExpirationTime = pt.get("cam.expirationTime", 1);
		mMaxGpsAge = pt.get("cam.maxGpsAge", 10);
		mMaxObd2Age = pt.get("cam.maxObd2Age", 10);
	}
};

/**
 * Class that handles the receiving, creating and sending of CA Messages.
 */
class CaService {
public:
	CaService(CaServiceConfig &config);
	~CaService();

	/** Sends a new CAM to LDM and DCC.	 */
	void send();

	/** Calculates the heading towards North based on the two specified coordinates.
	 *
	 * @param lat1 Latitude of coordinate 1.
	 * @param lon1 Longitude of coordinate 1.
	 * @param lat2 Latitude of coordinate 2.
	 * @param lon2 Longitude of coordinate 2.
	 * @return The heading in degrees.
	 */
	double getHeading(double lat1, double lon1, double lat2, double lon2);

	/** Calculates the distance between the two specified coordinates
	 *
	 * @param lat1 Latitude of coordinate 1.
	 * @param lon1 Longitude of coordinate 1.
	 * @param lat2 Latitude of coordinate 2.
	 * @param lon2 Longitude of coordinate 2.
	 * @return The distance in meters.
	 */
	double getDistance(double lat1, double lon1, double lat2, double lon2);

private:
	/** Receives incoming CAMs from DCC and forwards them to LDM.
	 * Receives serialized DATA packages from DCC, deserializes it, and forwards the contained serialized CAM to LDM.
	 */
	void receive();

	/** Logs the delay of the specified (serialized) CAM between its creation time and the current time.
	 *
	 * @param serializedCam
	 */
	void logDelay(std::string serializedCam);

	/** Sends information about why a CAM was triggered to LDM.
	 * Sends serialized camInfo to LDM, including the current time and the specified reason for triggering.
	 * @param triggerReason Difference in time, heading, position, or speed.
	 * @param delta Specifies how big the difference in time, heading, position, or speed is.
	 */
	void sendCamInfo(std::string triggerReason, double delta);

	/** Periodically checks the rules for triggering a new CAM.
	 * Checks every 100ms whether the difference in time, heading, position, or speed requires the triggering of a new CAM. Reasons for triggering are:
	 * The time difference since the last triggered CAM is at least one second.
	 * The heading since the last CAM changed by more than four degrees.
	 * The position since the last CAM changed by more than five meters.
	 * The speed since the last CAM changed by more than 1m/s.
	 * @param ec Boost error code
	 */
	void triggerCam(const boost::system::error_code &ec);

	/** Generates a new CAM.
	 * The new CAM includes the MAC address as stationId, an increasing but not unique ID, a current time stamp, and the latest GPS and OBD2 data if it is not too old (as configured).
	 * @return The newly generated CAM.
	 */
	camPackage::CAM generateCam();

	/** Generates a new data package that includes the specified CAM.
	 * The specified CAM is serialized and included in the new data package.
	 * Additionally, the ID, type, priority, creation time and life time of the specified CAM are included.
	 * @param cam The CAM to be included in the data package.
	 * @return The newly generated data package.
	 */
	dataPackage::DATA generateData(camPackage::CAM cam);

	/** Receives new GPS data from the GPS module.
	 *
	 */
	void receiveGpsData();

	/** Receives new OBD2 data from the OBD2 module.
	 *
	 */
	void receiveObd2Data();

	GlobalConfig mGlobalConfig;
	CaServiceConfig mConfig;

	CommunicationReceiver* mReceiverFromDcc;
	CommunicationSender* mSenderToDcc;
	CommunicationSender* mSenderToLdm;

	CommunicationReceiver* mReceiverGps;
	CommunicationReceiver* mReceiverObd2;

	boost::thread* mThreadReceive;
	boost::thread* mThreadGpsDataReceive;
	boost::thread* mThreadObd2DataReceive;

	LoggingUtility* mLogger;

	boost::asio::io_service mIoService;
	boost::asio::deadline_timer* mTimer;

	long mIdCounter;
	double mCamTriggerInterval;

	gpsPackage::GPS mLatestGps;
	/**
	 * True iff a GPS was received and it is not too old.
	 */
	bool mGpsValid;		//true if GPS was received and it's not too old
	std::mutex mMutexLatestGps;

	obd2Package::OBD2 mLatestObd2;
	/**
	 * True iff a OBD2 was received and it is not too old.
	 */
	bool mObd2Valid;
	std::mutex mMutexLatestObd2;

	camPackage::CAM mLastSentCam;
};

/** @} */ //end group
#endif
