//code partly from http://softwaresouls.com/softwaresouls/2012/03/05/linux-c-dynamixel-reading-and-writing-example/

#include <iostream>
#include <string.h>
#include <sys/ioctl.h>

#include "SerialPort.h"


int SerialPort::connect() {
	return connect("//dev//ttyUSB0");
}

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
	fileDescriptor = open(device, O_RDWR | O_NOCTTY | O_NDELAY | O_FSYNC );

	// clear terminalAttributes data
	memset(&terminalAttributes, 0, sizeof(struct termios));

	/*	http://linux.die.net/man/3/termios
	 *
	 *  control modes: c_cflag flag constants:
	 *
	 * 57600 bauds
	 * 8 bits per word
	 * Ignore modem control lines.
	 * Enable receiver.
	 */

	terminalAttributes.c_cflag = B57600 | CS8 | CLOCAL | CREAD;

	/*
	 * input modes: c_iflag flag constants:
	 *
	 * Ignore framing errors and parity errors.
	 * (XSI) Map NL to CR-NL on output.
	 */
	terminalAttributes.c_iflag = IGNPAR |  ONLCR;

	/*
	 * output modes: flag constants defined in POSIX.1
	 *
	 * Enable implementation-defined output processing.
	 */

	terminalAttributes.c_oflag = OPOST;

	/*
	 * Canonical and noncanonical mode
	 *
	 * min time
	 * min bytes to read
	 */

	//terminalAttributes.c_lflag = ICANON;
	terminalAttributes.c_cc[VTIME] = 0;
	terminalAttributes.c_cc[VMIN] = 1;

	/*
	 * http://linux.die.net/man/3/tcsetattr
	 * Set the port to our state
	 *
	 * the change occurs immediately
	 */

	tcsetattr(fileDescriptor, TCSANOW, &terminalAttributes);

	/*
	 * http://linux.die.net/man/3/tcflush
	 *
	 * flushes data written but not transmitted.
	 * flushes data received but not read.
	 */

	tcflush(fileDescriptor, TCOFLUSH);
	tcflush(fileDescriptor, TCIFLUSH);

	return fileDescriptor;
}

void SerialPort::disconnect(void) {
    close(fileDescriptor);
    printf("\nPort 1 has been CLOSED and %d is the file description\n", fileDescriptor);
}

int SerialPort::readSpeed() {	//FIXME
	char buffer [30] = {0};
	int n = write(fileDescriptor, "010d1\r", 6);
	std::cout << "after write, n = " << n << std::endl;
	n = read(fileDescriptor, buffer, sizeof buffer);
	std::cout << "after read, n = " << n << std::endl;
	std::cout << buffer << std::endl;
	return n;	//TODO return speed not n
}

int SerialPort::sendArray(unsigned char *buffer, int len) {
	int n=write(fileDescriptor, buffer, len);
	return n;
}

int SerialPort::getArray (unsigned char *buffer, int len) {
	int n=read(fileDescriptor, buffer, len);
	return n;
}

void SerialPort::clear()
{
	tcflush(fileDescriptor, TCIFLUSH);
	tcflush(fileDescriptor, TCOFLUSH);
}

int SerialPort::bytesToRead()
{
	int bytes=0;
	ioctl(fileDescriptor, FIONREAD, &bytes);

	return bytes;
}
