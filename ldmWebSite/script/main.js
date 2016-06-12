containers = [];

/**
 *updateFunction is repeatedly called to provide data for this div. should take a callback which shall be called with the retrived data 
 * @param updateFunction fn(callback)
 */
function Container(name,updateFunction,color,updateInterval){
	this.updateInterval = updateInterval || 1000;
	var color = color || "#555555";
	this.id = name;
	this.updateFunction = updateFunction;
	this.intervalID = -1;
	var htmlstr = 
		'<div id="'+name+'" class="container dataContainer"> '+
        	'<h4 style="display:inline-block">'+name+'</h4>'+
        	'<div style="background:green" class="updateButton"></div>'+
        	'<table id="'+name+'_data">'+
				'<tr><td>loading</td><td>Data</td></tr>'+
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
	
	this.updateButton = div.children(".updateButton");
	this.enableUpdate = function(){
		if (this.intervalID === -1){
			this.intervalID=window.setInterval(
				function(){this.updateFunction(this.setTable);}.bind(this),
				this.updateInterval
			);
			this.updateButton.css("background","green");
		}
	}.bind(this);
	this.enableUpdate();
	
	this.disableUpdate = function(){
		window.clearInterval(this.intervalID);
		this.intervalID=-1;
		this.updateButton.css("background","red");
	}.bind(this);
	
	this.toggleUpdate= function(){
		if (this.intervalID ===-1){
			this.enableUpdate();
		} else {
			this.disableUpdate();
		}
	}.bind(this);
	this.updateButton.click(this.toggleUpdate);
}

function camUpdate(callback){
	requestCam(callback);
}


function initMap(){
	//init map
	var map = L.map('mapContainer');

	// create the tile layer with correct attribution
	var osmUrl="map/openstreetmap/{z}/{x}/{y}.png";
	//online map 'http://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png';
	//marbel chache file:///home/jonh/.local/share/marble/maps/earth/openstreetmap/{z}/{x}/{y}.png
	
	
	var osmAttrib='Map data cr <a href="http://openstreetmap.org">OpenStreetMap</a> contributors';
	var osm = new L.TileLayer(osmUrl, {minZoom: 1, maxZoom: 18, attribution: osmAttrib,trackResize:true});		

	// start the map in Paderborn
	map.setView(new L.LatLng(51.7315, 8.739),15);
	map.addLayer(osm);
	
	var pos =[51.7315, 8.739];
	var marker = L.marker([51.7315, 8.739]).addTo(map);
	var myIcon = L.icon({
	    iconUrl: 'image/marker/marker-icon-red.png',
	});
	
	//var marker2 = L.marker([50.505, 30.57], {icon: myIcon}).addTo(map);
	
	window.setInterval(function(){
//		requestCam(function(cam) {
//			if(cam.gps){
//				pos[0] = cam.gps.latitude;
//				pos[1] = cam.gps.longitude;
//			}
//			marker.setLatLng(pos);
//		})
		//marker2.setLatLng([pos[0]+0.001*(Math.random()+0.5),pos[1]]);
		map.invalidateSize();
		map.setView(pos);
	},1000);
	

	$("#mapWrapper").draggable().resizable();
}

$(document).ready(function(){
	
	initMap();
	
	var counter = 1;
	
	var carContainer = new Container("car", function(callback) {
		callback({speed:counter,rpm:counter++ * 36,driver:"alive"});
	},color="#ff2222");
	
	var dccContainer = new Container("dcc", function(callback) {
		callback({status:"running", queued:(counter*93)%77,state:(counter%2 === 0)?"busy":"relaxed",queueBE:2,queueBK:5,queueVI:0,queueVO:100});
	},color="#22ff22");
	
	lockLayout();
});

function createCamDiv(){
	cam = new Container("Cam", function(callback) {
		requestCam(callback);
	},color= "#2222ff");
}
