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


#include <iostream>
#include <string.h>
#include <sstream>
#include <sys/ioctl.h>

#include "SerialPort.h"

using namespace std;

//connect to serial device at specified port, return file descriptor (-1 = cannot open)
//Taken from http://softwaresouls.com/softwaresouls/2012/03/05/linux-c-dynamixel-reading-and-writing-example/
int SerialPort::connect(char *device) {
	struct termios terminalAttributes;

	/*
	 * http://linux.die.net/man/2/open
	 *
	 * Open the serial port
	 * read/write
	 * not become the process's controlling terminal
	 * When possible, the file is opened in nonblocking mode
	 *
	 */
	mFileDescriptor = open(device, O_RDWR | O_NOCTTY | O_NDELAY | O_FSYNC );

	// clear terminalAttributes data
	memset(&terminalAttributes, 0, sizeof(struct termios));

	/*	http://linux.die.net/man/3/termios
	 *
	 * 	control modes: c_cflag flag constants:
	 *
	 * 	38400 bauds
	 * 	8 bits per word
	 * 	Ignore modem control lines.
	 * 	Enable receiver.
	 */

	terminalAttributes.c_cflag = B38400 | CS8 | CLOCAL | CREAD;

	/*
	 * input modes: c_iflag flag constants:
	 *
	 * Ignore framing errors and parity errors.
	 * (XSI) Map NL to CR-NL on output.
	 */
	terminalAttributes.c_iflag = IGNBRK;

	terminalAttributes.c_oflag = 0;

	/*
	 * Canonical and noncanonical mode
	 *
	 * min time
	 * min bytes to read
	 */
	terminalAttributes.c_cc[VMIN] = 0;
	terminalAttributes.c_cc[VTIME] = 5;

	/*
	 * http://linux.die.net/man/3/tcsetattr
	 * Set the port to our state
	 *
	 * the change occurs immediately
	 */

	tcsetattr(mFileDescriptor, TCSANOW, &terminalAttributes);

	/*
	 * http://linux.die.net/man/3/tcflush
	 *
	 * flushes data written but not transmitted.
	 * flushes data received but not read.
	 */

	tcflush(mFileDescriptor, TCOFLUSH);
	tcflush(mFileDescriptor, TCIFLUSH);

	return mFileDescriptor;
}

void SerialPort::disconnect(void) {
    close(mFileDescriptor);
    printf("\nPort 1 has been CLOSED and %d is the file description\n", mFileDescriptor);
}

void SerialPort::init() {
	cout << "Initializing OBD2" << endl;

	write(mFileDescriptor, "atz\r", 4);		//reset all (AT Z + return)
	write(mFileDescriptor, "AT SP0\r", 7);	//auto search for suitable protocol
	write(mFileDescriptor, "ATDP\r", 5);	//verify protocol
}

//reads speed and returns speed in m/s
double SerialPort::readSpeed() {
	int speedKmh = -1;
	double speedMs = -1;
	stringstream stream;

	//request & read speed
	write(mFileDescriptor, "010D\r", 5);
	read(mFileDescriptor, mBuffer, sizeof mBuffer);
	string response(mBuffer);

	//correct response format: "010D\r41 0D XX \r\r>" with XX being the speed in hex
	if (response.compare(0,4,"010D") == 0 && response.compare(5,5,"41 0D") == 0) {
		//convert hex to decimal
		stream << hex << response.substr(11,2);
		stream >> speedKmh;
		speedMs = (double)speedKmh / 3.6;			//convert km/h to m/s

		//print
		cout << "Speed in hex: " << response.substr(11,2) << endl;
		cout << "Speed in km/h: " << speedKmh << endl;
		cout << "Speed in m/s: " << speedMs << endl;
	}
	else {	//incorrect (eg. "010D\rSEARCHING...\r\r")
		cout << "Invalid speed. Plug in OBD2 and start engine." << endl;
	}

	memset(mBuffer, 0, BUFFER_SIZE);				//reset buffer
	return speedMs;
}

//reads and returns RPM (not included in ETSI standard)
int SerialPort::readRpm() {	//TODO: reading RPM did not work during the outdoor test, check response format again
	int rpm = -1;
	stringstream stream;

	//request & read RPM
	write(mFileDescriptor, "010C\r", 5);
	read(mFileDescriptor, mBuffer, sizeof mBuffer);
	string response(mBuffer);

	//correct response format: "010C\r41 0C XX XX \r\r>" with XX XX being the RPM*4 in hex
	if (response.compare(0,4,"010C") == 0 && response.compare(5,5,"41 0C") == 0) {
		//convert hex to actual decimal RPM
		stream << hex << response.substr(11,2) << response.substr(14,2);
		stream >> rpm;
		rpm = rpm / 4;

		//print
		cout << "RPM in hex: " << response.substr(11,2) << response.substr(14,2) << endl;
		cout << "RPM: " << rpm << endl;
	}
	else {	//incorrect (eg. "010D\rSEARCHING...\r\r")
		cout << "Invalid RPM. Plug in OBD2 and start engine." << endl;
	}

	memset(mBuffer, 0, BUFFER_SIZE);				//reset buffer
	return rpm;
}
