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
#include <sys/stat.h>
#include <unistd.h>
#include <ios>
#include <iostream>
#include <fstream>
#include <stdlib.h> 
#include <string.h>
#include <uci.h>
#include <algorithm>


using boost::property_tree::ptree;

std::string get_openc2x_path(std::string logBasePath, std::string expName, int expNo){
	return logBasePath + "/" + expName + "/" + std::to_string(expNo) + "/";
}

void lookup_section(ptree * pt, uci_section *s){
	struct uci_option *o;
	struct uci_element* e2;

	std::string section_type(s->type);
	uci_foreach_element(&s->options, e2) {
					//std::cout << "parse element" << std::endl;
					o = uci_to_option(e2);
					std::string name(e2->name);
					std::string value(o->v.string);
					//std::cout << section_type << name << value << std::endl;
					pt->put(section_type + "." + name, value);
	}
}




void lookup_config (ptree * pt, std::string config_name, std::string config_type){
	
	std::cout << "lookup" << config_name << "->" << config_type << std::endl;
	struct uci_context* uci;
	struct uci_package* package;
	struct uci_element* e;

	std::string package_name;
	if(config_type == "global" || config_type.empty()){
		package_name = "openc2x";
	}else{
		package_name = "openc2x_" + config_type;
	}

	uci = uci_alloc_context();
	
	if (uci == NULL){
		std::cerr << "UCI context not found" << std::endl;
		exit(1);
	}
	std::cout << "context allocated" << std::endl;
	if (uci_load(uci, package_name.c_str(), &package)) {
		uci_free_context(uci);
		std::cerr << "package " << package_name << " not found" << std::endl;
		exit(1);
	}
	std::cout << "package loaded" << std::endl;
	if(config_name.empty()){
		uci_foreach_element(&package->sections, e) {
			struct uci_section *s = uci_to_section(e);	
			std::string section_type(s->type);
			
			if((!config_type.empty() && section_type == config_type) || config_type.empty()){
				lookup_section(pt, s);
			}	
		}
	}else{
		struct uci_section *s = uci_lookup_section(uci, package, config_name.c_str());
		if(s==NULL){
			std::cerr << "UCI section not found" << std::endl;
			exit(1);
		}
		lookup_section(pt, s);
	}
	std::cout << "content loaded" << std::endl;
	uci_unload(uci, package);
	uci_free_context(uci);
	
}
	
ptree load_config_tree(){
	ptree pt;
	lookup_config(&pt, "", "global");
	std::string config_name;
	config_name = pt.get("global.config_name", "");
	std::cout << "Using config: " << config_name << std::endl;

	std::cout << "Start lookup" << std::endl;
	if(config_name.empty()){
		config_name = "default";
	}
	lookup_config(&pt, config_name, "common");
	lookup_config(&pt, config_name, "cam");
	lookup_config(&pt, config_name, "dcc");
	lookup_config(&pt, config_name, "denm");
	lookup_config(&pt, config_name, "httpServer");
	lookup_config(&pt, config_name, "obd2");
	lookup_config(&pt, config_name, "gps");
	lookup_config(&pt, config_name, "ldm");
	
	std::cout << "End lookup" << std::endl;

	//lookup_global(pt);
	for (auto& subtree: pt){
		for (auto& child: subtree.second){
			//std::cout << subtree.first << "." << child.first << " - " << child.second.data() << std::endl;
		}
	}
	return pt;
}





/**
 * Struct that hold the global configuration.
 * The configuration is defined in <a href="../../common/config/config.xml">common/config/config.xml</a>
 * @ingroup common
 */
struct GlobalConfig {
	/** Log Path */
	std::string mLogBasePath;
	/** name of experiment */
	std::string mExpName;
	/** number of experiment */
	int mExpNo;
	/** name of Ethernet device */
	std::string mEthernetDevice;
	/** MAC address of mEthernetDevice*/
	std::string mMac;
	/** country code uf system */
	std::string mCountryCode;
	/** power of wlan sender */
	int mTxPower;
	/** frequency of wlan sender */
	int mFrequency;
	
	uint32_t mStationID;
	
	/** IP in OCB*/
	std::string mOcbIP;
	
	// Taken from http://stackoverflow.com/questions/7326123/convert-mac-address-stdstring-into-uint64-t
	uint64_t stringToMac(std::string const& s) {
	    unsigned char a[6];
	    int last = -1;
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
	
	
	uint32_t stringToMac32(std::string const& s) {
	    unsigned char a[6];
	    int last = -1;
	    int rc = sscanf(s.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx%n",
	                    a + 0, a + 1, a + 2, a + 3, a + 4, a + 5,
	                    &last);
	    if(rc != 6 || s.size() != last) {
	    	std::cerr << "Invalid MAC" << std::endl;
			exit(1);
	    }
	    return
	        uint32_t(a[2]) << 24 |
	        uint32_t(a[3]) << 16 |
	        uint32_t(a[4]) << 8 |
	        uint32_t(a[5]);
	}
	
	void loadConfig(const std::string &application_name) {
		ptree pt = load_config_tree();
		mLogBasePath = pt.get("common.logBasePath", "/var/log/openc2x");
		mExpName = pt.get("global.config_name", "");
		if(mExpName==""){
			mExpName="FACTORY_CONFIG";
		}
		if(!exists(mLogBasePath)){
			createDir(mLogBasePath);
		}
		std::string appPath = mLogBasePath + "/" + mExpName;
		if(!exists(appPath)){
			createDir(appPath);
		}
		std::string expPath = "";
		int tmpExpNo = 0;
		bool foundEmpty = false;
		while(!foundEmpty){
			tmpExpNo++;
			expPath = appPath + "/" + std::to_string(tmpExpNo);
			if(exists(expPath)){
				if(!exists(expPath + "/." + application_name)){
					foundEmpty = true;
				}
			}else{
				createDir(expPath);
				foundEmpty = true;
			}
		}
		mExpNo = tmpExpNo;
		createFile(expPath + "/." + application_name);
		
		mEthernetDevice = pt.get("common.ethernetDevice", "notDefined");
		mCountryCode = pt.get("common.countryCode", "DE");
		mTxPower = pt.get("common.txPower", 100);
		mStationID = pt.get("common.stationId", 0);
		mFrequency = pt.get("common.frequency", 5900);
		
		
		//get MAC Address
		std::string file = std::string("/sys/class/net/")+ mEthernetDevice + "/address";
		std::ifstream infile(file);
		getline(infile, mMac);

		//TODO remove 
		if(mMac == "00:12:3f:13:4d:90"){
			mOcbIP = "192.168.99.103";
		}else if(mMac == "00:0d:b9:3f:7c:bc"){
			mOcbIP = "192.168.99.101";
		}else if(mMac == "00:0d:b9:48:97:60"){
			mOcbIP = "192.168.99.104";
		}else if(mMac == "00:0d:b9:48:51:ec"){
			mOcbIP = "192.168.99.105";
		}else if(mMac == "00:0d:b9:48:4f:b8"){
			mOcbIP = "192.168.99.106";
		}else if(mMac == "00:0d:b9:2e:d1:40"){
			mOcbIP = "192.168.99.205";
		}
		
		
		//ENDTODO

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
		if(mStationID == 0){
			mStationID = stringToMac32(mMac);
			std::cout << "The identified mac as integer: " << std::to_string(mStationID) << std::endl;
		}
		
	}
	
	
	inline bool exists (const std::string& name) {
		struct stat buffer;   
		return (stat (name.c_str(), &buffer) == 0); 
	}
	
	inline void createDir (const std::string& name) {
		const int dir_err = mkdir(name.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if (-1 == dir_err)	{
			std::cout << "Error creating directory " << name << std::endl;
			exit(1);
		} 
	}
	
	inline void createFile (const std::string& name) {
		std::cout << "Write lock: " << name << std::endl;
		std::ofstream file;
		file.open (name, std::ios::out);
	}
};
