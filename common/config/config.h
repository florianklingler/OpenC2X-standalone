#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

struct GlobalConfig {
	int mExpNo;		//number of experiment
	std::string mEthernetDevice;
	std::string mMac;

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
		}
	}
};
