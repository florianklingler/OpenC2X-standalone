
#ifndef STATE_H_
#define STATE_H_

#include "DccConfig.h"
#include <map>

class State {
public:
	size_t asStateId;
	double asChanLoad;

	DCC_NDL_Param<dcc_Mechanism_t> asDcc;
	DCC_NDL_Param<double> asTxPower;
	DCC_NDL_Param<double> asPacketInterval;
	DCC_NDL_Param<double> asDatarate;
	DCC_NDL_Param<double> asCarrierSense;

	State(DCC_State_Config &cfg) {
		asStateId = cfg.asStateId;
		asChanLoad = cfg.asChanLoad;
		asDcc = DCC_NDL_Param<dcc_Mechanism_t>(cfg.asDcc);
		asTxPower = DCC_NDL_Param<double>(cfg.asTxPower);
		asPacketInterval = DCC_NDL_Param<double>(cfg.asPacketInterval);
		asDatarate = DCC_NDL_Param<double>(cfg.asDatarate);
		asCarrierSense = DCC_NDL_Param<double>(cfg.asCarrierSense);
	}
	~State() {}
};

typedef std::map<size_t, State> States;

#endif /* STATE_H_ */
