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

void Server::requestData() {
	string request, reply;
	int i = 0;
	while(1) {
		request = to_string(i);
		reply = mClientLdm->sendRequest(request, 2500, 3);
		if (reply != "") {
			//TODO: process reply
		}
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
	serv.requestData();
}
