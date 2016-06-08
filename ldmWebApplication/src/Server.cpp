#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE

#include "Server.h"
#include "pbjson.hpp"
#include "crow_all.h"

INITIALIZE_EASYLOGGINGPP

Server::Server() {
	std::string moduleName = "WebApplication";
	mClientLdm = new CommunicationClient(moduleName, "6789");
	mLogger = new LoggingUtility(moduleName);
}

Server::~Server() {
	delete mClientLdm;
	delete mLogger;
}

//requests data from all LDM tables and forwards it to the website
void Server::requestData() {
	std::string request, reply;
	std::string serializedData;
	dataPackage::LdmData ldmData;
	while(1) {
		//get all CAMs from LDM
		reply = mClientLdm->sendRequest("CAM", "", 2500, 3);
		if (reply != "") {
			ldmData.ParseFromString(reply);

			//convert to JSON
			std::string json = "{\"type\":\"CAM\",\"number\":" + std::to_string(ldmData.data_size()) + ",\"msgs\":[";
			for (int i=0; i<ldmData.data_size(); i++) {
				std::string tempJson;
				std::string serializedCam = ldmData.data(i);
				camPackage::CAM cam;
				cam.ParseFromString(serializedCam);
				pbjson::pb2json(&cam, tempJson);
				if (i > 0) {
					json += "," + tempJson;
				}
				else {
					json += tempJson;
				}
			}
			json += "]}";
			//TODO: send json
		}


		//get all DENMs from LDM
		reply = mClientLdm->sendRequest("DENM", "", 2500, 3);
		if (reply != "") {
			ldmData.ParseFromString(reply);

			//convert to JSON
			std::string json = "{\"type\":\"DENM\",\"number\":" + std::to_string(ldmData.data_size()) + ",\"msgs\":[";
			for (int i=0; i<ldmData.data_size(); i++) {
				std::string tempJson;
				std::string serializedDenm = ldmData.data(i);
				denmPackage::DENM denm;
				denm.ParseFromString(serializedDenm);
				pbjson::pb2json(&denm, tempJson);
				if (i > 0) {
					json += "," + tempJson;
				}
				else {
					json += tempJson;
				}
			}
			json += "]}";
			//TODO: send json
		}


		//get all GPSs from LDM
		reply = mClientLdm->sendRequest("GPS", "", 2500, 3);
		if (reply != "") {
			ldmData.ParseFromString(reply);

			//convert to JSON
			std::string json = "{\"type\":\"GPS\",\"number\":" + std::to_string(ldmData.data_size()) + ",\"msgs\":[";
			for (int i=0; i<ldmData.data_size(); i++) {
				std::string tempJson;
				std::string serializedGps = ldmData.data(i);
				gpsPackage::GPS gps;
				gps.ParseFromString(serializedGps);
				pbjson::pb2json(&gps, tempJson);
				if (i > 0) {
					json += "," + tempJson;
				}
				else {
					json += tempJson;
				}
			}
			json += "]}";
			//TODO: send json
		}


		//get all OBD2s from LDM
		reply = mClientLdm->sendRequest("OBD2", "", 2500, 3);
		if (reply != "") {
			ldmData.ParseFromString(reply);

			//convert to JSON
			std::string json = "{\"type\":\"OBD2\",\"number\":" + std::to_string(ldmData.data_size()) + ",\"msgs\":[";
			for (int i=0; i<ldmData.data_size(); i++) {
				std::string tempJson;
				std::string serializedObd2 = ldmData.data(i);
				obd2Package::OBD2 obd2;
				obd2.ParseFromString(serializedObd2);
				pbjson::pb2json(&obd2, tempJson);
				if (i > 0) {
					json += "," + tempJson;
				}
				else {
					json += tempJson;
				}
			}
			json += "]}";
			//TODO: send json
		}


		//get all dccInfos from LDM
		reply = mClientLdm->sendRequest("dccInfo", "", 2500, 3);
		if (reply != "") {
			ldmData.ParseFromString(reply);

			//convert to JSON
			std::string json = "{\"type\":\"dccInfo\",\"number\":" + std::to_string(ldmData.data_size()) + ",\"msgs\":[";
			for (int i=0; i<ldmData.data_size(); i++) {
				std::string tempJson;
				std::string serializedDccInfo = ldmData.data(i);
				infoPackage::DccInfo dccInfo;
				dccInfo.ParseFromString(serializedDccInfo);
				pbjson::pb2json(&dccInfo, tempJson);
				if (i > 0) {
					json += "," + tempJson;
				}
				else {
					json += tempJson;
				}
			}
			json += "]}";
			//TODO: send json
		}


		//get all camInfos from LDM
		reply = mClientLdm->sendRequest("camInfo", "", 2500, 3);
		if (reply != "") {
			ldmData.ParseFromString(reply);

			//convert to JSON
			std::string json = "{\"type\":\"camInfo\",\"number\":" + std::to_string(ldmData.data_size()) + ",\"msgs\":[";
			for (int i=0; i<ldmData.data_size(); i++) {
				std::string tempJson;
				std::string serializedCamInfo = ldmData.data(i);
				infoPackage::CamInfo camInfo;
				camInfo.ParseFromString(serializedCamInfo);
				pbjson::pb2json(&camInfo, tempJson);
				if (i > 0) {
					json += "," + tempJson;
				}
				else {
					json += tempJson;
				}
			}
			json += "]}";
			//TODO: send json
		}

		sleep(1);
	}
}


void Server::run(){
	crow::SimpleApp app;

	CROW_ROUTE(app, "/add_json")
	.methods("POST"_method)
	([](const crow::request& req){
	    auto x = crow::json::load(req.body);
	    if (!x)
	        return crow::response(400);
	    int sum = x["a"].i()+x["b"].i();
	    std::ostringstream os;
	    os << sum;
	    return crow::response{os.str()};
	});
	app.port(1880).run();

}

int main(){
	Server serv;
	serv.run();
	serv.requestData();
}
