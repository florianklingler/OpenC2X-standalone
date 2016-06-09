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


class Server {
public:

	Server();
	virtual ~Server();

	std::string requestCam(std::string condition);
	std::string requestDenm(std::string condition);
	std::string requestGps(std::string condition);
	std::string requestObd2(std::string condition);
	std::string requestDccInfo(std::string condition);
	std::string requestCamInfo(std::string condition);

	std::string myMac();

private:
	GlobalConfig mConfig;

	CommunicationClient* mClientLdm;

	LoggingUtility* mLogger;
};
#endif /* SERVER_H_ */
