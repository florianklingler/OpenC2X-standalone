/*
 * Server.cpp
 *
 *  Created on: May 25, 2016
 *      Author: root
 */

#include "Server.h"
#include "crow.h"

Server::Server() {
	// TODO Auto-generated constructor stub

}

Server::~Server() {
	// TODO Auto-generated destructor stub
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
	    os << "{" << sum << "}";
	    crow::response resp{os.str()};
	    resp.add_header("Access-Control-Allow-Origin","*");
		return resp;
	});
	app.port(1188).run();

}

int main(){
	Server serv;
	serv.run();
}
