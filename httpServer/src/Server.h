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

class Server {
public:

	Server(GlobalConfig config);
	virtual ~Server();
	/**short desc.
	 * long desc
	 * @param condition
	 * @return
	 */
	std::string requestCam(std::string condition);
	std::string requestDenm(std::string condition);
	std::string requestGps(std::string condition);
	std::string requestObd2(std::string condition);
	std::string requestDccInfo(std::string condition);
	std::string requestCamInfo(std::string condition);

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
