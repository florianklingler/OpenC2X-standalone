/*
 * Server.cpp
 *
 *  Created on: May 25, 2016
 *      Author: root
 */
#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE

#include "Server.h"
//#include "crow_all.h"

INITIALIZE_EASYLOGGINGPP

Server::Server() {
	mClientLdm = new CommunicationClient("WebApplication", "6789");
	mClientLdm->init();
}

Server::~Server() {
	delete mClientLdm;
}

void Server::request() {
	int i = 0;
	while(1) {
		std::cout << "Send request" << std::endl;
		mClientLdm->sendRequest(to_string(i));
		sleep(1);
		i++;
	}
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
	serv.request();
}
