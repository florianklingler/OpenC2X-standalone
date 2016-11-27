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

/** @addtogroup website
 *  @{
 */

/**
 *  time after which CAMs are marked as old and grayed out on the map in seconds.
 */
camTimeout = 60 //seconds

denmData = {
	mymac : "",
	denms : new Map(),
	refreshRate : 1000,
	lastUpdate : 0,

	init : function(){
		if (this.mymac == ""){//is not initalised
			//start updating
			// window.setInterval(function(){
			// 	denmData.updateDenms();
			// },denmData.refreshRate);
			//get own mac
			requestMyMac(function(data) {
				this.mymac = data.myMac;
			}.bind(this));
			return false;
		} else {
			return true;
		}
	},
	updateDenms: function(){
		if (this.lastUpdate+this.refreshRate < new Date().getTime()){
			requestDenm(this.digestDenms);
		}
	},
	digestDenms : function(data){
		if (denmData.init()){ //is initalised
			data.msgs.forEach(function(denm) {
				denmData.denms.set(denm.header.stationID.toString(),denm);
			})
		}
	},
	getLastOwnDenm : function(callback){
		denmData.init();
		var table = {};
		var denm = denmData.denms.get(denmData.mymac);
		var isVehicle = (denm.msg.managementContainer.stationType === 5) ? "Vehicle" : "RSU";
		table["header"] = {
							"protocolVersion" : denm.header.protocolVersion,
							"messageID" : denm.header.messageID,
							"stationID" : denm.header.stationID
						};
		table["denm"] = {
							"latitude" : denm.msg.managementContainer.latitude,
							"longitude" : denm.msg.managementContainer.longitude,
							"altitude" : denm.msg.managementContainer.altitude,
							"stationType" : isVehicle
						};
		if(callback){
			callback(table);
		} else {
			//console.log(camData.mymac + " and map" + camData.cams)
			return table;
		}
	},
	getReceivedDenmDetail : function(callback){
		if(denmData.init()){
			var table={};
			denmData.denms.forEach(function(denm, stationID) {
				if (stationID == denmData.mymac) {
					// do nothing
				}
				else {
					//console.log(cam.coop.camParameters.basicContainer.latitude)
					var isVehicle = (denm.msg.managementContainer.stationType === 5) ? "Vehicle" : "RSU";
					table[stationID] = {
						"protocolVersion" : denm.header.protocolVersion,
						"messageID" : denm.header.messageID,
						"stationID" : denm.header.stationID,
						"latitude" : denm.msg.managementContainer.latitude,
						"longitude" : denm.msg.managementContainer.longitude,
						"altitude" : denm.msg.managementContainer.altitude,
						"stationType" : isVehicle
					}
					
				}
				
			})
			callback(table);
		}
	},
};

/**
 * holds and updates most recent cam data for each station id
 */
camData = {
	mymac : "",
	cams : new Map(),
	refreshRate : 1000,
	lastUpdate : 0,

	/**
	 * checks whether camData is inialised and initialises it if not
	 * @returns {Boolean} is initalised
	 */
	init : function(){
		if (this.mymac == ""){//is not initalised
			//get own mac
			requestMyMac(function(data) {
				this.mymac = data.myMac;
			}.bind(this));
			return false;
		} else {
			return true;
		}
	},
	/**
	 * takes a response object from "webAppQuerys.js" and updates camDatas cam map
	 * @param data
	 */
	digestCams : function(data){
		if (camData.init()){ //is initalised
			data.msgs.forEach(function(cam) {
//				if(camData.cams.get(cam.header.stationID)){
//					if (camData.cams.get(cam.stationID).createTime < cam.createTime){
//						camData.cams.set(cam.stationID,cam);
//					}
//				} else {
					camData.cams.set(cam.header.stationID.toString(),cam);
//				}
			})
//			console.log("printing the cams map")
//			console.log(camData.cams)
		}
	},
	
	/**
	 * if the last update is old enougth this functions triggers a new update and a new request is send to httpServer.
	 */
	updateCams: function(){
		if (this.lastUpdate+this.refreshRate < new Date().getTime()){
			requestCam(this.digestCams);
		}
	},
	getLastOwnCam : function(callback){
		camData.init();
		var table = {};
		var cam = camData.cams.get(camData.mymac);
		var isVehicle = (cam.coop.camParameters.basicContainer.stationType === 5) ? "Vehicle" : "RSU";
		table["header"] = {
							"protocolVersion" : cam.header.protocolVersion,
							"messageID" : cam.header.messageID,
							"stationID" : cam.header.stationID
						};
		if (isVehicle.includes("RSU")) {
			table["cam"] = {
						"genDeltaTime" : cam.coop.genDeltaTime,
						"stationType" : isVehicle,
						"latitude" : cam.coop.camParameters.basicContainer.latitude/10000000,
						"longitude" : cam.coop.camParameters.basicContainer.longitude/10000000,
						"altitude" : cam.coop.camParameters.basicContainer.altitude
						};
		} else {
			table["cam"] = {
						"genDeltaTime" : cam.coop.genDeltaTime,
						"stationType" : isVehicle,
						"latitude" : cam.coop.camParameters.basicContainer.latitude/10000000,
						"longitude" : cam.coop.camParameters.basicContainer.longitude/10000000,
						"altitude" : cam.coop.camParameters.basicContainer.altitude,
						"speed" : cam.coop.camParameters.highFreqContainer.basicVehicleHighFreqContainer.speed,
						"heading" : cam.coop.camParameters.highFreqContainer.basicVehicleHighFreqContainer.heading
						};
		}


		
		if(callback){
			callback(table);
		} else {
			//console.log(camData.mymac + " and map" + camData.cams)
			return table;
		}
	},
	getReceivedCamDetail : function(callback){
		if(camData.init()){
			var table={};
			camData.cams.forEach(function(cam, stationID) {
				if (stationID == camData.mymac) {
					// do nothing
				}
				else {
					//console.log(cam.coop.camParameters.basicContainer.latitude)
					var isVehicle = (cam.coop.camParameters.basicContainer.stationType === 5) ? "Vehicle" : "RSU";
					if (isVehicle.includes("RSU")) {
						table[stationID] = {
							"protocolVersion" : cam.header.protocolVersion,
							"messageID" : cam.header.messageID,
							"stationID" : cam.header.stationID,
							"genDeltaTime" : cam.coop.genDeltaTime,
							"stationType" : isVehicle,
							"latitude" : cam.coop.camParameters.basicContainer.latitude/10000000,
							"longitude" : cam.coop.camParameters.basicContainer.longitude/10000000,
							"altitude" : cam.coop.camParameters.basicContainer.altitude
						}
					} else {
						table[stationID] = {
							"protocolVersion" : cam.header.protocolVersion,
							"messageID" : cam.header.messageID,
							"stationID" : cam.header.stationID,
							"genDeltaTime" : cam.coop.genDeltaTime,
							"stationType" : isVehicle,
							"latitude" : cam.coop.camParameters.basicContainer.latitude/10000000,
							"longitude" : cam.coop.camParameters.basicContainer.longitude/10000000,
							"altitude" : cam.coop.camParameters.basicContainer.altitude,
							"speed" : cam.coop.camParameters.highFreqContainer.basicVehicleHighFreqContainer.speed,
							"heading" : cam.coop.camParameters.highFreqContainer.basicVehicleHighFreqContainer.heading
						}
					}
					
				}
				
			})
			callback(table);
		}
	},	
	getCamOverview : function(callback){
		if(camData.init()){
			var table= {}; //own mac is at top
			camData.cams.forEach(function(cam, stationID) {
				if (stationID == camData.mymac){
					table["self"] = {"createTime" : cam.createTime};
				}else{
					table[stationID] = {"createTime" : cam.createTime};
				} 
			})
			callback(table);
		}
	},
};


/** Container that handles the updating of the corresponding div on the webpage.
 * updateFunction is repeatedly called to provide data for this div. 
 * The Function hould take a callback which shall be called with the retrived data.
 * @param updateFunction fn(callback)
 * @param name name and id of the container and corresponding div
 * @param color OPTIONAL HEX value of the color of the div
 * @param updateInterval OPTIONAL time between updates in ms
 */
function Container(name,updateFunction,color,updateInterval){
	this.updateInterval = updateInterval || 1000;
	var color = color || "#555555";
	this.id = name;
	this.updateFunction = updateFunction;
	this.intervalID = -1;
	closeThisContainer = function(objId){
		$(objId).remove();
	};
	var htmlstr = 
		'<div id="'+name+'" class="container dataContainer"> '+
			'<a href="javascript:void(0)" onclick="closeThisContainer('+name+')"><i class="fa fa-times" aria-hidden="true" style="float: right;"></i></a>'+
        	'<h4 style="display:inline-block; width: 100%; text-align: center;">'+name+'</h4>'+
        	//'<div style="background:green" class="updateButton"></div>'+
        	'<table id="'+name+'_data">'+
				'<tr><td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Loading</td><td>Data...&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td></tr>'+
        	'</table>'+
    	'</div>';
	$("body").append(htmlstr);
	var div = $("#"+name).draggable().resizable();
	div.css("border-color",color);
	div.css("background",increase_brightness(color, 70));
	this.dataTable = $("#"+name+"_data");
	this.setTable = function(data){
		$("#"+this.id+"_data").html(objectToTable(data));
	}.bind(this);
	
	//this.updateButton = div.children(".updateButton");
	this.enableUpdate = function(){
		if (this.intervalID === -1){
			this.intervalID=window.setInterval(
				function(){this.updateFunction(this.setTable);}.bind(this),
				this.updateInterval
			);
	//		this.updateButton.css("background","green");
		}
	}.bind(this);
	this.enableUpdate();
	
//	this.disableUpdate = function(){
//		window.clearInterval(this.intervalID);
//		this.intervalID=-1;
//		this.updateButton.css("background","red");
//	}.bind(this);
//	
//	this.toggleUpdate= function(){
//		if (this.intervalID ===-1){
//			this.enableUpdate();
//		} else {
//			this.disableUpdate();
//		}
//	}.bind(this);
//	this.updateButton.click(this.toggleUpdate);
}


/**
 * Initialises the Map.
 */
function initMap(){
	//init map
	map = L.map('mapContainer');

	// create the tile layer with correct attribution
	var osmUrl="map/openstreetmap/{z}/{x}/{y}.png";
	//online map 'http://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png';
	//marbel chache file:///home/jonh/.local/share/marble/maps/earth/openstreetmap/{z}/{x}/{y}.png
	
	
	var osmAttrib='Map data cr <a href="http://openstreetmap.org">OpenStreetMap</a> contributors';
	var osm = new L.TileLayer(osmUrl, {minZoom: 1, maxZoom: 18, attribution: osmAttrib,trackResize:true});		

	// start the map in Paderborn
	viewPosition = [51.7315, 8.739];
	map.setView(viewPosition,15);
	map.addLayer(osm);
	
	markers = [];
	
	window.setInterval(function(){
		if (camData.init()){//not uninitalised
			markers.forEach(function(marker) {
				map.removeLayer(marker);
			})
			var redMarker = L.icon({
				    iconUrl: 'image/marker/marker-icon-red.png',
				    iconSize: [25,41],
				    iconAnchor: [12,41],
                                    popupAnchor:  [0, -20]
				});
			var redMarkerPale = L.icon({
			    iconUrl: 'image/marker/marker-icon-red-pale.png',
			    iconSize: [25,41],
			    iconAnchor: [12,41],
                            popupAnchor:  [0, -20]
			});
            var blueMarker = L.icon({
			    iconUrl: 'image/marker/marker-icon-blue.png',
			    iconSize: [25,41],
			    iconAnchor: [12,41],
                            popupAnchor:  [0, -20]
			});

			//var ownCam = camData.getLastOwnCam();
			camData.cams.forEach(function(cam, key) {
				if ( key == camData.mymac){//own cam
					//if(cam.gps){
						var lat = cam.coop.camParameters.basicContainer.latitude/10000000;
						var lon = cam.coop.camParameters.basicContainer.longitude/10000000;
						viewPosition = [lat, lon];
						var marker = L.marker(viewPosition,{icon:blueMarker}).addTo(map);
						//station id popup
						marker.bindPopup(cam.header.stationID);
						marker.on('mouseover', function(e){
						    marker.openPopup();
						});
						markers.push(marker);
					//}
				} else {//other cams : red marker
					//if(cam.gps){
						//if(cam.createTime +camTimeout*1000000000 < ownCam.createTime){//other cam is old
						//	var icon = redMarkerPale;
						//} else {//cam is fresh
							var icon = redMarker;
						//}
						var lat = cam.coop.camParameters.basicContainer.latitude/10000000;
						var lon = cam.coop.camParameters.basicContainer.longitude/10000000;
						var marker = L.marker([lat, lon],{icon: icon}).addTo(map);
						marker.bindPopup(cam.header.stationID);
						marker.on('mouseover', function(e){
						    marker.openPopup();
						});
						markers.push(marker);
					//}
				}
			})
		}
		map.invalidateSize();
		map.setView(viewPosition);
	},1300);
	

	$("#mapWrapper").draggable().resizable();
}

$(document).ready(function(){
	initMap();

	window.setInterval(function(){
	 	camData.updateCams();
	},camData.refreshRate);
	
	window.setInterval(function(){
		denmData.updateDenms();
	},denmData.refreshRate);

// 	var counter = 1;
	
//	var carContainer = new Container("car", function(callback) {
//		callback({speed:counter,rpm:counter++ * 36,driver:"alive"});
//	},color="#ff2222");
//	
//	var dccContainer = new Container("dcc", function(callback) {
//		callback({status:"running", queued:(counter*93)%77,state:(counter%2 === 0)?"busy":"relaxed",queueBE:2,queueBK:5,queueVI:0,queueVO:100});
//	},color="#22ff22");
	
});


/** Open when someone clicks on the button element */
function openNav() {
    document.getElementById("myNav").style.width = "50%";
}

/** Close when someone clicks on the "x" symbol inside the overlay */
function closeNav() {
    document.getElementById("myNav").style.width = "0%";
}

/** @} */ // end of group
