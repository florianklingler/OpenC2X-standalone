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
			std::cerr << "Invalid MAC. Check common/config/config.xml" << std::endl;
			exit(1);
		}

		// station id
		mStationID = pt.get("global.stationId", 12345789); //stringToMac(mMac);
	}
};
