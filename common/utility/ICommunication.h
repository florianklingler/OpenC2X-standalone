#ifndef ICOMMUNICATION_H_
#define ICOMMUNICATION_H_

#include <iostream>

using namespace std;


class ICommunication {
public:
	virtual ~ICommunication(){}
    virtual string process(string message) = 0;
};

typedef  string (ICommunication::*ICommunicationFunction)(string message);

#endif
