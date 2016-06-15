#ifndef OBD2SERVICE_H_
#define OBD2SERVICE_H_

#include "SerialPort.h"
#include <utility/CommunicationSender.h>
#include <utility/LoggingUtility.h>
#include <buffers/build/obd2.pb.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/asio.hpp>
#include <string>
#include <config/config.h>

struct Obd2Config {
	bool mSimulateData;
	char* mDevice;
	int mFrequency;

	void loadConfigXML(const std::string &filename) {
		boost::property_tree::ptree pt;
		read_xml(filename, pt);

		mSimulateData = pt.get("obd2.SimulateData", true);
		std::string device = pt.get("obd2.Device", "//dev//ttyUSB0");
		mDevice = strdup(device.c_str());
		mFrequency = pt.get("obd2.Frequency", 100);
	}
};

class Obd2Service {
public:
	Obd2Service(Obd2Config &config);
	~Obd2Service();

	void receiveData(const boost::system::error_code &ec, SerialPort* serial);
	double simulateSpeed();
	void simulateData(const boost::system::error_code &ec);
	void sendToServices(obd2Package::OBD2 obd2);

private:
	Obd2Config mConfig;
	GlobalConfig mGlobalConfig;

	CommunicationSender* mSender;
	LoggingUtility* mLogger;

	//for simulation only
	std::default_random_engine mRandNumberGen;
	std::bernoulli_distribution mBernoulli;
	std::uniform_real_distribution<double> mUniform;

	boost::asio::io_service mIoService;
	boost::asio::deadline_timer* mTimer;
};

#endif
