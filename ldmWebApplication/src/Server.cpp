#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE

#include "Server.h"
//#include "crow_all.h"

INITIALIZE_EASYLOGGINGPP

Server::Server() {
	string moduleName = "WebApplication";
	mClientLdm = new CommunicationClient(moduleName, "6789");
	mLogger = new LoggingUtility(moduleName);
}

Server::~Server() {
	delete mClientLdm;
	delete mLogger;
}

//requests data from all LDM tables and forwards it to the website
void Server::requestData() {
	string request, reply;
	string serializedData;
	dataPackage::LdmData ldmData;
	while(1) {
		//get all CAMs from LDM
		reply = mClientLdm->sendRequest("CAM", "", 2500, 3);
		//TODO: do we need if (reply != "")?
		ldmData.ParseFromString(reply);
		//TODO: send to website. maybe iterate over data and parse individual CAMs
//		for (int i=0; i<ldmData.data_size(); i++) {
//			string serializedCam = ldmData.data(i);
//			camPackage::CAM cam;
//			cam.ParseFromString(serializedCam);
//		}

		//get all DENMs from LDM
		reply = mClientLdm->sendRequest("DENM", "", 2500, 3);
		//TODO: do we need if (reply != "")?
		ldmData.ParseFromString(reply);
		//TODO: send to website

		//get all GPSs from LDM
		reply = mClientLdm->sendRequest("GPS", "", 2500, 3);
		//TODO: do we need if (reply != "")?
		ldmData.ParseFromString(reply);
		//TODO: send to website

		//get all OBD2s from LDM
		reply = mClientLdm->sendRequest("OBD2", "", 2500, 3);
		//TODO: do we need if (reply != "")?
		ldmData.ParseFromString(reply);
		//TODO: send to website

		//get all dccInfos from LDM
		reply = mClientLdm->sendRequest("dccInfo", "", 2500, 3);
		//TODO: do we need if (reply != "")?
		ldmData.ParseFromString(reply);
		//TODO: send to website

		//get all camInfos from LDM
		reply = mClientLdm->sendRequest("camInfo", "", 2500, 3);
		//TODO: do we need if (reply != "")?
		ldmData.ParseFromString(reply);
		//TODO: send to website
	}
	sleep(1);
}


//void Server::run(){
//	crow::SimpleApp app;
//
//	CROW_ROUTE(app, "/add_json")
//	.methods("POST"_method)
//	([](const crow::request& req){
//	    auto x = crow::json::load(req.body);
//	    if (!x)
//	        return crow::response(400);
//	    int sum = x["a"].i()+x["b"].i();
//	    std::ostringstream os;
//	    os << "{" << sum << "}";
//	    crow::response resp{os.str()};
//	    resp.add_header("Access-Control-Allow-Origin","*");
//		return resp;
//	});
//	app.port(1188).run();
//
//}

int main(){
	Server serv;
//	serv.run();
	serv.requestData();
}
