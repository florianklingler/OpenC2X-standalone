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


#ifndef SERIALPORT_H_
#define SERIALPORT_H_

#include <stdio.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>

#define BUFFER_SIZE 30

/** Helper class that handles the actual connection to OBD2 via serial port.
 *
 */
class SerialPort {
private:
	 int mFileDescriptor;
	 char mBuffer[BUFFER_SIZE] = {0};

   public:
	 void init();

	 /** Connects to serial device.
	  *  Connects to serial device at specified port and returns the file descriptor (-1 = cannot open).
	  *  @param device The serial device to connect to.
	  */
	 int connect(char *device);

	 /** Disconnects from serial device.
	  *
	  */
	 void disconnect(void);

	 /** Reads speed.
	  *	 Requests speed from OBD2 device and returns it in m/s.
	  */
	 double readSpeed();

	 /** Reads rpm.
	  *	 Requests rpm from OBD2 device and returns it.
	  *	 Note: rpm is not included in the ETSI standard.
	  */
	 int readRpm();
};


#endif /* SERIALPORT_H_ */
