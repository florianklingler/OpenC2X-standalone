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
