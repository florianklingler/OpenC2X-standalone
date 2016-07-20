// This file is part of OpenC2X.
//
// OpenC2X is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OpenC2X is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OpenC2X.  If not, see <http://www.gnu.org/licenses/>.
//
// Authors:
// Sven Laux <slaux@mail.uni-paderborn.de>
// Gurjashan Singh Pannu <gurjashan.pannu@ccs-labs.org>
// Stefan Schneider <stefan.schneider@ccs-labs.org>
// Jan Tiemann <janhentie@web.de>


#include "Utils.h"
#include <sstream>
#include <chrono>

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

int64_t Utils::currentTime() {
	return chrono::high_resolution_clock::now().time_since_epoch() / chrono::nanoseconds(1);
}
