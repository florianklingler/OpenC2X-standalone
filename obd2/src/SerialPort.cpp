#include <iostream>
#include <string.h>
#include <sstream>
#include <sys/ioctl.h>

#include "SerialPort.h"

using namespace std;

//TODO: test configuration in conenct and init

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
//	fileDescriptor = open(device, O_RDWR | O_NOCTTY | O_NDELAY | O_FSYNC );
	mFileDescriptor = open(device, O_RDWR | O_NOCTTY | O_SYNC );		//pg2014

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
	terminalAttributes.c_iflag = IGNPAR |  ONLCR;
//	terminalAttributes.c_iflag = IGNBRK;	//pg2014

	/*
	 * output modes: flag constants defined in POSIX.1
	 *
	 * Enable implementation-defined output processing.
	 */

	terminalAttributes.c_oflag = OPOST;
//	terminalAttributes.c_oflag = 0;		//pg2014

	/*
	 * Canonical and noncanonical mode
	 *
	 * min time
	 * min bytes to read
	 */

	//terminalAttributes.c_lflag = ICANON;
	terminalAttributes.c_cc[VTIME] = 0;
	terminalAttributes.c_cc[VMIN] = 1;
//	terminalAttributes.c_cc[VMIN] = 0;	//pg2014
//	terminalAttributes.c_cc[VTIME] = 5;

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

//	write(mFileDescriptor, "atz\r", 4);		//reset all (AT Z + return)
//	read(mFileDescriptor, mBuffer, sizeof mBuffer);
//	cout << "AT Z: " << mBuffer << endl;
//	memset(mBuffer, 0, BUFFER_SIZE);

	write(mFileDescriptor, "AT SP0\r", 7);	//auto search for suitable protocol
	read(mFileDescriptor, mBuffer, sizeof mBuffer);
	cout << "AT SP0: " << mBuffer << endl;	//FIXME: first char gets replaced by > because buffer ends with "\r>" => goes to start of line and prints >
	memset(mBuffer, 0, BUFFER_SIZE);

//	write(mFileDescriptor, "ate0\r", 5);	//disable echo
//	read(mFileDescriptor, mBuffer, sizeof mBuffer);
//	cout << "AT E0: " << mBuffer << endl;
//	memset(mBuffer, 0, BUFFER_SIZE);

	write(mFileDescriptor, "ATDP\r", 5);	//verify protocol
	read(mFileDescriptor, mBuffer, sizeof mBuffer);
	cout << "AT DP: " << mBuffer << endl;
	memset(mBuffer, 0, BUFFER_SIZE);
}

//reads speed and returns speed in m/s
double SerialPort::readSpeed() {
	int speedKmh = -1;
	double speedMs = -1;
	stringstream stream;

	//request & read speed
	write(mFileDescriptor, "010D\r", 5);
	read(mFileDescriptor, mBuffer, sizeof mBuffer);

	//speed is returned in 6 digits, last 2 digits are speed in hex (410D--)
	if (mBuffer[6] == '\0') {						//7th digit not empty => invalid speed (eg. "Searching..")
		//convert hex to decimal
		stream << hex << mBuffer[4] << mBuffer[5];	//convert hex to dec (digits 5 and 6)
		stream >> speedKmh;
		speedMs = (double)speedKmh / 3.6;						//convert km/h to m/s

		//print
		cout << "Speed: " << mBuffer << endl;
		cout << "Speed-hex: " << mBuffer[4] << mBuffer[5] << endl;
		cout << "Speed-ms: " << speedMs << endl;
		cout << "Speed-kmh: " << speedKmh << endl;
	}
	else {
		cout << "Invalid speed; buffer: " << mBuffer << endl;
	}

	memset(mBuffer, 0, BUFFER_SIZE);				//reset buffer
	return speedMs;
}

