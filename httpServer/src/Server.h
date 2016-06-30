#ifndef SERVER_H_
#define SERVER_H_

#include <config/config.h>
#include <utility/CommunicationClient.h>
#include <utility/LoggingUtility.h>
#include <buffers/build/cam.pb.h>
#include <buffers/build/denm.pb.h>
#include <buffers/build/gps.pb.h>
#include <buffers/build/obd2.pb.h>
#include <buffers/build/dccInfo.pb.h>
#include <buffers/build/camInfo.pb.h>
#include <buffers/build/ldmData.pb.h>
#include <google/protobuf/text_format.h>
#include <mutex>

struct WebApplicationConfig {
	int mTimeout;

	void loadConfigXML(const std::string &filename) {
		boost::property_tree::ptree pt;
		read_xml(filename, pt);

		mTimeout = pt.get("webApplication.timeout", 100);

	}
};

/** A Server that connects to LDMs via ZMQ and exposes it's Data via http.
 * Uses the Crow Framework (<a href="https://github.com/ipkn/crow"> git link</a>) for the http service.
 * @todo write about protobuf to json libary
 */
class Server {
public:

	Server(GlobalConfig config);
	virtual ~Server();

	/** Gets CAMs from LDM and converts them to JSON.
	 * Requests CAMs from LDM via ZMQ and converts them from Protobuffer to JSON.
	 *
	 * Condition is not working at the moment. Always returns latest CAM from each known Station ID.
	 * @param condition if =="latest" returns only the latest Cams else returns all Cams
	 * @return JSON String of a Array of CAM(s)
	 */
	std::string requestCam(std::string condition);

	/** Gets DENMs from LDM and converts them to JSON.
	 * Requests DENMs from LDM via ZMQ and converts them from Protobuffer to JSON.
	 *
	 * Condition is not working at the moment. Always returns latest DENMs from each known Station ID.
	 * @param condition if =="latest" returns only the latest DENMs else returns all DENMs
	 * @return JSON String of a Array of CAM(s)
	 */
	std::string requestDenm(std::string condition);

	/**
	 * @todo check whether function is needed/working
	 * @deprecated not tested
	 * @param condition
	 * @return
	 */
	std::string requestGps(std::string condition);

	/**
	 * @todo check whether function is needed/working
	 * @deprecated not tested
	 * @param condition
	 * @return
	 */
	std::string requestObd2(std::string condition);

	/** Gets a JSON String containing information about the local DCC from LDM.
	 * Returns a JSON String which encodes a array containing four information objects
	 * about the four access categorys of DCC from LDM via ZMQ.
	 *
	 * Condition is not working at the moment.
	 * @param condition condition if =="latest" returns only the latest else return all
	 * @return JSON String
	 */
	std::string requestDccInfo(std::string condition);

	/** Gets a JSON String containing Information about the last created
	 * CAM by the local CAM Service from LDM.
	 * Contains Information like triggering reason oder creation time. Obtained from LDM via ZMQ.
	 *
	 * @param condition
	 * @return JSON string about the last CAM
	 */
	std::string requestCamInfo(std::string condition);

	/**returns own mac address of the used ethernet device defined in the global config.
	 * @return String of MAC address
	 */
	std::string myMac();

private:
	GlobalConfig mGlobalConfig;
	WebApplicationConfig mLocalConfig;

	CommunicationClient* mClientCam;
	CommunicationClient* mClientDenm;
	CommunicationClient* mClientGps;
	CommunicationClient* mClientObd2;
	CommunicationClient* mClientCamInfo;
	CommunicationClient* mClientDccInfo;

	std::mutex mMutexCam;
	std::mutex mMutexDenm;
	std::mutex mMutexGps;
	std::mutex mMutexObd2;
	std::mutex mMutexCamInfo;
	std::mutex mMutexDccInfo;

	LoggingUtility* mLogger;
};
#endif /* SERVER_H_ */
