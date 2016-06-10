#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE

#include "Server.h"
#include "pbjson.hpp"
#include "crow_all.h"
#include <utility/CommunicationSender.h>
#include <buffers/build/trigger.pb.h>

INITIALIZE_EASYLOGGINGPP

Server::Server() {
	try {
		mConfig.loadConfigXML("../../common/config/config.xml");
	}
	catch (std::exception &e) {
		std::cerr << "Error while loading config.xml: " << e.what() << std::endl;
	}

	std::string moduleName = "WebApplication";
	mClientLdm = new CommunicationClient(moduleName, "6789");
	mLogger = new LoggingUtility(moduleName);
}

Server::~Server() {
	delete mClientLdm;
	delete mLogger;
}

//requests all CAMs from LDM
std::string Server::requestCam(std::string condition) {
	std::string request, reply;
	std::string serializedData;
	dataPackage::LdmData ldmData;
	//get all CAMs from LDM
	reply = mClientLdm->sendRequest("CAM", condition, 1000, 1);
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
std::string Server::requestDenm(std::string condition) {
	std::string request, reply;
	std::string serializedData;
	dataPackage::LdmData ldmData;
	//get all DENMs from LDM
	reply = mClientLdm->sendRequest("DENM", condition, 100, 1);
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
std::string Server::requestGps(std::string condition) {
	std::string request, reply;
	std::string serializedData;
	dataPackage::LdmData ldmData;
	//get all GPSs from LDM
	reply = mClientLdm->sendRequest("GPS", condition, 100, 1);
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
std::string Server::requestObd2(std::string condition) {
	std::string request, reply;
	std::string serializedData;
	dataPackage::LdmData ldmData;
	//get all OBD2s from LDM
	reply = mClientLdm->sendRequest("OBD2", condition, 100, 1);
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
std::string Server::requestDccInfo(std::string condition) {
	std::string request, reply;
	std::string serializedData;
	dataPackage::LdmData ldmData;;
	//get all dccInfos from LDM
	reply = mClientLdm->sendRequest("dccInfo", condition, 100, 1);
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
std::string Server::requestCamInfo(std::string condition) {
	std::string request, reply;
	std::string serializedData;
	dataPackage::LdmData ldmData;
	//get all camInfos from LDM
	reply = mClientLdm->sendRequest("camInfo", condition, 100, 1);
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

//returns own mac address from global config
std::string Server::myMac() {
	return "{\"myMac\":\"" + mConfig.mMac + "\"}";
}


int main(){
	crow::SimpleApp app;

	//ldm requests
	Server* server = new Server();
	//CAM
	CROW_ROUTE(app, "/request_cam")
	.methods("POST"_method)
	([server](const crow::request& req){

		auto request = crow::json::load(req.body);
		if (!request)
			return crow::response(400);
		std::string condition = "";
		std::string reply = "";

		if(request.has("condition")) {
			condition = request["condition"].s();
		}

		reply = server->requestCam(condition);

	    auto resp = crow::response{reply};
	    resp.add_header("Access-Control-Allow-Origin","*");
		return resp;
	});

	//DENM
	CROW_ROUTE(app, "/request_denm")
	.methods("POST"_method)
	([server](const crow::request& req){

		auto request = crow::json::load(req.body);
		if (!request)
			return crow::response(400);
		std::string condition = "";
		std::string reply = "";

		if(request.has("condition")) {
			condition = request["condition"].s();
		}

		reply = server->requestDenm(condition);

	    auto resp = crow::response{reply};
	    resp.add_header("Access-Control-Allow-Origin","*");
		return resp;
	});

	//GPS
	CROW_ROUTE(app, "/request_gps")
	.methods("POST"_method)
	([server](const crow::request& req){

		auto request = crow::json::load(req.body);
		if (!request)
			return crow::response(400);
		std::string condition = "";
		std::string reply = "";

		if(request.has("condition")) {
			condition = request["condition"].s();
		}

		reply = server->requestGps(condition);

	    auto resp = crow::response{reply};
	    resp.add_header("Access-Control-Allow-Origin","*");
		return resp;
	});

	//OBD2
	CROW_ROUTE(app, "/request_obd2")
	.methods("POST"_method)
	([server](const crow::request& req){

		auto request = crow::json::load(req.body);
		if (!request)
			return crow::response(400);
		std::string condition = "";
		std::string reply = "";

		if(request.has("condition")) {
			condition = request["condition"].s();
		}

		reply = server->requestObd2(condition);

		std::cout << "Response: " << reply << std::endl;
	    auto resp = crow::response{reply};
	    resp.add_header("Access-Control-Allow-Origin","*");
		return resp;
	});

	//DccInfo
	CROW_ROUTE(app, "/request_dccinfo")
	.methods("POST"_method)
	([server](const crow::request& req){

		auto request = crow::json::load(req.body);
		if (!request)
			return crow::response(400);
		std::string condition = "";
		std::string reply = "";

		if(request.has("condition")) {
			condition = request["condition"].s();
		}

		reply = server->requestDccInfo(condition);

		std::cout << "Response: " << reply << std::endl;
	    auto resp = crow::response{reply};
	    resp.add_header("Access-Control-Allow-Origin","*");
		return resp;
	});

	//CamInfo
	CROW_ROUTE(app, "/request_caminfo")
	.methods("POST"_method)
	([server](const crow::request& req){

		auto request = crow::json::load(req.body);
		if (!request)
			return crow::response(400);
		std::string condition = "";
		std::string reply = "";

		if(request.has("condition")) {
			condition = request["condition"].s();
		}

		reply = server->requestCamInfo(condition);

		std::cout << "Response: " << reply << std::endl;
	    auto resp = crow::response{reply};
	    resp.add_header("Access-Control-Allow-Origin","*");
		return resp;
	});

	//denm triggering
	CommunicationSender* senderToDenm = new CommunicationSender("WebApplication", "1111");
	CROW_ROUTE(app, "/trigger_denm")
	.methods("POST"_method)
	([senderToDenm](const crow::request& req){

		auto request = crow::json::load(req.body);
		if (!request)
			return crow::response(400);
		triggerPackage::TRIGGER trigger;
		std::string serializedTrigger;
		std::string content = "";

		if(request.has("content")) {
			content = request["content"].s();
		}

		trigger.set_content(content);
		trigger.SerializeToString(&serializedTrigger);

		senderToDenm->send("TRIGGER", serializedTrigger);

	    auto resp = crow::response{"Triggered DENM"};
	    resp.add_header("Access-Control-Allow-Origin","*");
		return resp;
	});

	//gets own mac
	CROW_ROUTE(app, "/my_mac")
	.methods("GET"_method)
	([server](){
		auto resp = crow::response{server->myMac()};
		resp.add_header("Access-Control-Allow-Origin","*");
		return resp;
	});

	app.port(1188).multithreaded().run();

	delete server;
	delete senderToDenm;
}
