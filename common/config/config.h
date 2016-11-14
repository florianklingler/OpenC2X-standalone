#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <iostream>

/**
 * Struct that hold the global configuration.
 * The configuration is defined in <a href="../../common/config/config.xml">common/config/config.xml</a>
 * @ingroup common
 */
struct GlobalConfig {
	/** number of experiment */
	int mExpNo;
	/** name of Ethernet device */
	std::string mEthernetDevice;
	/** MAC address of mEthernetDevice*/
	std::string mMac;
	//TODO: GSP: Remove mMac once mStationID is used as ID everywhere.
	uint64_t mStationID;

	// Taken from http://stackoverflow.com/questions/7326123/convert-mac-address-stdstring-into-uint64-t
	uint64_t stringToMac(std::string const& s) {
	    unsigned char a[6];
	    uint last = -1;
	    int rc = sscanf(s.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx%n",
	                    a + 0, a + 1, a + 2, a + 3, a + 4, a + 5,
	                    &last);
	    if(rc != 6 || s.size() != last) {
	    	std::cerr << "Invalid MAC" << std::endl;
			exit(1);
	    }
	    return
	        uint64_t(a[0]) << 40 |
	        uint64_t(a[1]) << 32 |
	        uint64_t(a[2]) << 24 |
	        uint64_t(a[3]) << 16 |
	        uint64_t(a[4]) << 8 |
	        uint64_t(a[5]);
	}

	void loadConfigXML(const std::string &filename) {
		boost::property_tree::ptree pt;
		read_xml(filename, pt);

		mExpNo = pt.get("global.expNo", 1);
		mEthernetDevice = pt.get("global.ethernetDevice", "notDefined");

		//get MAC Address
		std::string file = std::string("/sys/class/net/")+ mEthernetDevice + "/address";
		std::ifstream infile(file);
		getline(infile, mMac);

		//check for right MAC format copied from http://stackoverflow.com/questions/4792035/how-do-you-validate-that-a-string-is-a-valid-mac-address-in-c
		int i = 0;
		int s = 0;
		const char* mac = mMac.c_str();
		std::cout <<"The identified mac: "<< mMac.c_str() << std::endl;
		while (*mac) {
		   if (isxdigit(*mac)) {
			  i++;
		   }
		   else if (*mac == ':' || *mac == '-') {
			  if (i == 0 || i / 2 - 1 != s)
				break;
			  ++s;
		   }
		   else {
			   s = -1;
		   }
		   ++mac;
		}
		if (!(i == 12 && (s == 5 || s == 0))){
			mMac = "invalid MAC";
			std::cerr << "Invalid MAC" << std::endl;
			exit(1);
		}

		// station id
		mStationID = pt.get("global.stationId", 12345789); //stringToMac(mMac);
	}
};
