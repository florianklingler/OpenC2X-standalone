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
	static Server* getInstance();

	std::string requestCAMs();
	std::string requestDENMs();
	std::string requestGPSs();
	std::string requestOBD2s();
	std::string requestDCCINFOs();
	std::string requestCAMINFOs();

private:
	static Server* _instance;

	Server();
	virtual ~Server();

	CommunicationClient* mClientLdm;

	LoggingUtility* mLogger;

	class CGuard {
		public:
		   ~CGuard() {
			  if( NULL != Server::_instance ) {
				 delete Server::_instance;
				 Server::_instance = NULL;
			  }
		   }
	};
};
#endif /* SERVER_H_ */
