//each container class div needs and unique id

containers = [];
layoutLocked = false;

/**
 * saves to current layout to localstorage on client side
 */
function saveLayout(){
	var containers = $(".container");
	var containerLayouts = {ids:[]};
	
	for (var i = 0; i< containers.length;i++){
		var con = containers[i];
		var config = {
			offset: $("#"+con.id).offset(),
			height: $("#"+con.id).height(),
			width: $("#"+con.id).width()
		}
		console.log(con);
		containerLayouts["ids"].push(con.id);
		containerLayouts[con.id] =config;
	}
	console.log("layout JSON String:" +JSON.stringify(containerLayouts));
	localStorage.setItem("containerLayouts",JSON.stringify(containerLayouts));
}


/**
 * loads and applys layout configs from jsonString
 */
function loadLayoutFromJSON(jsonString){
	containerLayouts = JSON.parse(jsonString);
	
	containerLayouts.ids.forEach(function(id){
		var config = containerLayouts[id]; 
		if($("#"+id).length === 0){ //container is not created
			if(typeof window["create"+id+"Container"] != 'undefined'){ //creation Function is defined
				window["create"+id+"Container"]() 
			}
		}
		$("#"+id).offset(config.offset),
		$("#"+id).height(config.height),
		$("#"+id).width(config.width)
		
	})
}


/**
 * loads and applys layout configs peviously saved via saveLayout()
 */
function loadLayout(){
	var containerLayoutsJSON = localStorage.getItem("containerLayouts");
	if(containerLayoutsJSON == null){
		console.log("No layout saved.");
		return;
	}
	loadLayoutFromJSON(containerLayoutsJSON);
}

function lockLayout(){
	$(".container").draggable("disable").resizable("disable");
}

function unlockLayout(){
	$(".container").draggable("enable").resizable("enable");
}

function toggleLayoutLock(){
	if(layoutLocked){
		unlockLayout();
	} else {
		lockLayout();
	}
	layoutLocked = !layoutLocked;
}

function createCamContainer(){
	if($("#Cam").length === 0){ //container is not created
		containers.push(new Container("Cam", camData.getLastOwnCam,color= "#0000ff"));
	}
}

function createDenmContainer(){
	if($("#Denm").length === 0){ //container is not created
	containers.push(new Container("Denm", function(callback) {
			requestDenm(callback);
		},color= "#FFFF00"));
	}
}

function createCamOverviewContainer(){
	if($("#CamOverview").length === 0){ //container is not created
		containers.push(new Container("CamOverview", function(callback) {
			camData.getCamOverview(callback);
		}, color="#8888ff"));
	}
}

function createCamInfoContainer(){
	if($("#CamInfo").length === 0){ //container is not created
		containers.push(new Container("CamInfo", function(callback) {
			requestCamInfo(callback);
		}, color="#5555ff"));
	}
}

function createDccInfoContainer(){
	if($("#DccInfo").length === 0){ //container is not created
		containers.push(new Container("DccInfo", function(callback) {
			requestDccInfo(callback);
		}, color="#FF7700"));
	}
}
