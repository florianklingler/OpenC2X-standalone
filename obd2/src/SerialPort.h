#ifndef SERIALPORT_H_
#define SERIALPORT_H_

#include <stdio.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>

#define BUFFER_SIZE 30

class SerialPort {
private:
	 int mFileDescriptor;
	 char mBuffer[BUFFER_SIZE] = {0};

   public:
	 int connect(char *device);
	 void disconnect(void);
	 void init();

	 double readSpeed();
};


#endif /* SERIALPORT_H_ */
