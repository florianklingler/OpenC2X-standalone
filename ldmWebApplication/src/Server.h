#ifndef SERVER_H_
#define SERVER_H_

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

	std::string requestCAMs(std::string condition);
	std::string requestDENMs(std::string condition);
	std::string requestGPSs(std::string condition);
	std::string requestOBD2s(std::string condition);
	std::string requestDCCINFOs(std::string condition);
	std::string requestCAMINFOs(std::string condition);

private:

	CommunicationClient* mClientLdm;

	LoggingUtility* mLogger;
};
#endif /* SERVER_H_ */
