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


/**  
 * @addtogroup layoutManager
 *  @{
 */

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
		//console.log(con);
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

/**
 * disables dragging and resizing of containers
 */
function lockLayout(){
	$(".container").draggable("disable").resizable("disable");
}

/**
 * enables dragging and resizing of containers
 */
function unlockLayout(){
	$(".container").draggable("enable").resizable("enable");
}

/**
 * switches between locked and unlocked Layout
 */
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

function createReceivedCamContainer(){
	if($("#ReceivedCam").length === 0){ //container is not created
		containers.push(new Container("ReceivedCam", function(callback) {
			camData.getReceivedCamDetail(callback);
		}, color="#aabbcc"));
	}
}

function createDenmContainer(){
	if($("#Denm").length === 0){ //container is not created
	containers.push(new Container("Denm", function(callback) {
			requestDenm(callback);
		},color= "#FFFF00"));
	}
}

function createReceivedDenmContainer(){
	if($("#ReceivedDenm").length === 0){ //container is not created
		containers.push(new Container("ReceivedDenm", function(callback) {
			camData.getReceivedCamDetail(callback);
		}, color="#66bb22"));
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

/**  @} */// end of group
