#include <iostream>
#include <string.h>
#include <sstream>
#include <sys/ioctl.h>

#include "SerialPort.h"

using namespace std;

//TODO: test configuration in connect and init

//connect to serial device at specified port, return file descriptor (-1 = cannot open)
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
//	mFileDescriptor = open(device, O_RDWR | O_NOCTTY | O_SYNC );				//pg2014

	// clear terminalAttributes data
	memset(&terminalAttributes, 0, sizeof(struct termios));

	/*	http://linux.die.net/man/3/termios
	 *
	 *  control modes: c_cflag flag constants:
	 *
	 * 38400 bauds
	 * 8 bits per word
	 * Ignore modem control lines.
	 * Enable receiver.
	 */

	terminalAttributes.c_cflag = B38400 | CS8 | CLOCAL | CREAD;

	/*
	 * input modes: c_iflag flag constants:
	 *
	 * Ignore framing errors and parity errors.
	 * (XSI) Map NL to CR-NL on output.
	 */
//	terminalAttributes.c_iflag = IGNPAR |  ONLCR;
	terminalAttributes.c_iflag = IGNBRK;	//pg2014

	/*
	 * output modes: flag constants defined in POSIX.1
	 *
	 * Enable implementation-defined output processing.
	 */

//	terminalAttributes.c_oflag = OPOST;
	terminalAttributes.c_oflag = 0;		//pg2014

	/*
	 * Canonical and noncanonical mode
	 *
	 * min time
	 * min bytes to read
	 */

	//terminalAttributes.c_lflag = ICANON;
//	terminalAttributes.c_cc[VTIME] = 0;
//	terminalAttributes.c_cc[VMIN] = 1;
	terminalAttributes.c_cc[VMIN] = 0;		//pg2014
	terminalAttributes.c_cc[VTIME] = 5;		//pg2014

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

