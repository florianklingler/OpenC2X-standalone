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

//requests all CAMs from LDM
std::string Server::requestCAMs(std::string condition) {
	std::string request, reply;
	std::string serializedData;
	dataPackage::LdmData ldmData;
	//get all CAMs from LDM
	reply = mClientLdm->sendRequest("CAM", condition, 2500, 3);
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
		return json;
	}
	return "";
}

//requests all DENMs from LDM
std::string Server::requestDENMs(std::string condition) {
	std::string request, reply;
	std::string serializedData;
	dataPackage::LdmData ldmData;
	//get all DENMs from LDM
	reply = mClientLdm->sendRequest("DENM", condition, 2500, 3);
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
		return json;
	}
	return "";
}

//requests all GPSs from LDM
std::string Server::requestGPSs(std::string condition) {
	std::string request, reply;
	std::string serializedData;
	dataPackage::LdmData ldmData;
	//get all GPSs from LDM
	reply = mClientLdm->sendRequest("GPS", condition, 2500, 3);
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
		return json;
	}
	return "";
}

//requests all OBD2s from LDM
std::string Server::requestOBD2s(std::string condition) {
	std::string request, reply;
	std::string serializedData;
	dataPackage::LdmData ldmData;
	//get all OBD2s from LDM
	reply = mClientLdm->sendRequest("OBD2", condition, 2500, 3);
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
		return json;
	}
	return "";
}

//requests all DCCINFOs from LDM
std::string Server::requestDCCINFOs(std::string condition) {
	std::string request, reply;
	std::string serializedData;
	dataPackage::LdmData ldmData;;
	//get all dccInfos from LDM
	reply = mClientLdm->sendRequest("dccInfo", condition, 2500, 3);
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
		return json;
	}
	return "";
}

//requests all CAMINFOs from LDM
std::string Server::requestCAMINFOs(std::string condition) {
	std::string request, reply;
	std::string serializedData;
	dataPackage::LdmData ldmData;
	//get all camInfos from LDM
	reply = mClientLdm->sendRequest("camInfo", condition, 2500, 3);
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
		return json;
	}
	return "";
}

int main(){
	crow::SimpleApp app;

	CROW_ROUTE(app, "/add_json")
	.methods("POST"_method)
	([](const crow::request& req){

		auto request = crow::json::load(req.body);
		if (!request)
			return crow::response(400);
		Server* server = new Server();
		std::string type = "";
		std::string condition = "";
		std::string reply = "";

		if(request.has("type")) {
			type = request["type"].s();
		}
		if(request.has("condition")) {
			condition = request["condition"].s();
		}

		if(type == "CAM") {
			reply = server->requestCAMs(condition);
		}
		if(type == "DENM") {
			reply = server->requestDENMs(condition);
		}
		if(type == "GPS") {
			reply = server->requestGPSs(condition);
		}
		if(type == "OBD2") {
			reply = server->requestOBD2s(condition);
		}
		if(type == "dccInfo") {
			reply = server->requestDCCINFOs(condition);
		}
		if(type == "camInfo") {
			reply = server->requestCAMINFOs(condition);
		}

		std::cout << "Response: " << reply << std::endl;
	    auto resp = crow::response{reply};
	    resp.add_header("Access-Control-Allow-Origin","*");
		return resp;
	});
	app.port(1188).run();
}
