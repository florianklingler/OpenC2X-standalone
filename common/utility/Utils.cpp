/*
 * Utils.cpp
 *
 *  Created on: Jul 12, 2016
 *      Author: pannu
 */

#include "Utils.h"
#include <sstream>

using namespace std;

Utils::Utils() {}

Utils::~Utils() {}

string Utils::readableTime(int64_t nanoTime) {
	char buffer[9];
	int64_t milliTime = nanoTime / (1*1000*1000);		//ns to ms (since epoch)
	time_t epochTime = milliTime / 1000;				//ms to s (since epoch)
	struct tm* timeinfo = localtime(&epochTime);
	strftime(buffer, 9, "%T", timeinfo);				//buffer contains time HH:MM:SS
	int ms = milliTime % epochTime;						//just the ms

	stringstream time;								//convert to string
	time << buffer << "." << ms;

	return time.str();
}
