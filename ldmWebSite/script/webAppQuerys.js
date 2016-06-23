
function requestMyMac(callback){
	$.get("http://localhost:1188/my_mac",
			function(data){
		callback(data);
	},"json");
}

/**
 * 
 * @param callback(latest cam msg)
 */
function requestCam(callback){
	$.post("http://localhost:1188/request_cam",JSON.stringify({condition:"latest"}),
			function(data/*status,xhr*/){
		//console.log("data: "+data);
		//console.log("status: "+status);
		callback(data);
	},"json");
}

function requestCamInfo(callback){
	$.post("http://localhost:1188/request_caminfo",JSON.stringify({condition:"latest"}),
			function(data){
		callback(data.msgs[data.msgs.length-1]);
	},"json");
}

function requestDenm(callback){
	$.post("http://localhost:1188/request_denm",JSON.stringify({condition:"latest"}),
			function(data){
                            var latestTime = 0;
                            var latestDenm;
                            data.msgs.forEach(function(denm){
                                if (denm.createTime > latestTime){
                                    latestDenm = denm;
                                    latestTime = denm.createTime;
                                }
                            });
		callback(latestDenm);
	},"json");
}

function requestGps(callback){
	$.post("http://localhost:1188/request_gps",JSON.stringify({condition:""}),
			function(data){
		callback(data.msgs[data.msgs.length-1]);
	},"json");
}

function requestObd2(callback){
	$.post("http://localhost:1188/request_obd2",JSON.stringify({condition:""}),
			function(data){
		callback(data.msgs[data.msgs.length-1]);
	},"json");
}

function requestDccInfo(callback){
	$.post("http://localhost:1188/request_dccinfo",JSON.stringify({condition:"latest"}),
			function(data){
		callback(data.msgs[data.msgs.length-1]);
	},"json");
}

function triggerDenm(){
	$.post("http://localhost:1188/trigger_denm",JSON.stringify({content: "triggered by GUI"}),	
			function(data,status,xhr){
		console.log("data: "+data);
		console.log("status: "+status);
	});
}