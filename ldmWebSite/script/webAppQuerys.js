
function myMac(){
	$.get("http://localhost:1188/my_mac",
			function(data,status,xhr){
		console.log("data: "+data);
		console.log("status: "+status);
	});
}

/**
 * 
 * @param callback(latest cam msg)
 */
function requestCam(callback){
	var post = $.post("http://localhost:1188/request_cam",JSON.stringify({condition:"latest"}),
			function(data/*status,xhr*/){
		//console.log("data: "+data);
		//console.log("status: "+status);
		callback(data.msgs[data.msgs.length-1]);
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
		callback(data.msgs[data.msgs.length-1]);
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