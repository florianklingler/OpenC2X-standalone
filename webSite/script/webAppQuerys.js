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
// Jan Tiemann <janhentie@web.de>

/** @addtogroup querys
 *  @{
 */

/**
 * requests local MAC address.
 * from httpServer via http and calls the callback function with the data.
 * @param callback  fn(data)
 */
function requestMyMac(callback){
	$.get("http://localhost:1188/my_mac",
			function(data){
		callback(data);
	},"json");
}

/**
 * requests the latest CAM from each station ID.
 *  from httpServer via http and calls the callback function with the data.
 * @param callback fn(data)
 */
function requestCam(callback){
	$.post("http://localhost:1188/request_cam",JSON.stringify({condition:"latest"}),
			function(data/*status,xhr*/){
		//console.log("data: "+data);
		//console.log("status: "+status);
		callback(data);
	},"json");
}

/**
 * requests the latest Information about the latest local CAM.
 * from httpServer via http and calls the callback function with the data.
 * @param callback fn(data)
 */
function requestCamInfo(callback){
	$.post("http://localhost:1188/request_caminfo",JSON.stringify({condition:"latest"}),
			function(data){
		callback(data.msgs[data.msgs.length-1]);
	},"json");
}

/**
 * requests the latest DENM from each station ID.
 * from httpServer via http and calls the callback function with the data.
 * @param callback fn(data)
 */
function requestDenm(callback){
	$.post("http://localhost:1188/request_denm",JSON.stringify({condition:"latest"}),
			function(data){
		callback(data);
	},"json");
}

/**
 * requests gps data.
 * from httpServer via http and calls the callback function with the data.
 * @deprecated untested
 * 
 * @param callback fn(data)
 */
function requestGps(callback){
	$.post("http://localhost:1188/request_gps",JSON.stringify({condition:""}),
			function(data){
		callback(data.msgs[data.msgs.length-1]);
	},"json");
}

/**
 * requests obd2 data.
 * from httpServer via http and calls the callback function with the data.
 * @deprecated untested
 * 
 * @param callback fn(data)
 */
function requestObd2(callback){
	$.post("http://localhost:1188/request_obd2",JSON.stringify({condition:""}),
			function(data){
		callback(data.msgs[data.msgs.length-1]);
	},"json");
}

/**
 * requests the latest dcc information.
 * about all 4 access categorys from httpServer via http and calls the callback function with the data.
 * @param callback fn(data)
 */
function requestDccInfo(callback){
	$.post("http://localhost:1188/request_dccinfo",JSON.stringify({condition:"latest"}),
			function(data){
		callback(data.msgs[data.msgs.length-1]);
	},"json");
}

function requestAC_BE(callback){
	$.post("http://localhost:1188/request_dccinfo",JSON.stringify({condition:"latest"}),
			function(data){
		var table = {};		
		table["AC_BE"] = {
							"access category " : data.msgs[0].Cat0.accessCategory,
							"state           " : data.msgs[0].Cat0.state,
							"time            " : data.msgs[0].Cat0.time,
							"token interval  " : data.msgs[0].Cat0.tokenInterval,
							"available tokens" : data.msgs[0].Cat0.availableTokens,
							"channel load    " : data.msgs[0].Cat0.channelLoad,
							"carrier sense   " : data.msgs[0].Cat0.carrierSense,
							"data rate       " : data.msgs[0].Cat0.datarate,
							"tx power        " : data.msgs[0].Cat0.txPower						
						};
		callback(table);
		//callback(data.msgs[data.msgs.length-1]);
	},"json");
}

function requestAC_VI(callback){
	$.post("http://localhost:1188/request_dccinfo",JSON.stringify({condition:"latest"}),
			function(data){
		var table = {};
		table["AC_VI"] = {
							"access category " : data.msgs[0].Cat2.accessCategory,
							"state           " : data.msgs[0].Cat2.state,
							"time            " : data.msgs[0].Cat2.time,
							"token interval  " : data.msgs[0].Cat2.tokenInterval,
							"available tokens" : data.msgs[0].Cat2.availableTokens,
							"channel load    " : data.msgs[0].Cat2.channelLoad,
							"carrier sense   " : data.msgs[0].Cat2.carrierSense,
							"data rate       " : data.msgs[0].Cat2.datarate,
							"tx power        " : data.msgs[0].Cat2.txPower						
						};
		callback(table);
		//callback(data.msgs[data.msgs.length-1]);
	},"json");
}

/**
 * triggers the creation of a DEN message.
 * via http request to httpServer
 */
function triggerDenm(){
	$.post("http://localhost:1188/trigger_denm",JSON.stringify({content: "triggered by GUI"}),	
			function(data,status,xhr){
		console.log("data: "+data);
		console.log("status: "+status);
	});
}

/** @} */ // end of group