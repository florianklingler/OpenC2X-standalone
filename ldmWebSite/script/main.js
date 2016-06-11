
/**
 *updateFunction is repeatedly called to provide data for this div. 
 * should take a callback which shall be called with the retrived data 
 */
function Container(name,updateFunction,updateInterval){
	updateInterval = updateInterval || 1000;
	this.id = name;
	var htmlstr = 
		'<div id="'+name+'" class="container dataContainer"> '+
        	'<h4>'+name+'</h4>'+
        	'<table id="'+name+'_data">'+
				'<tr><td>loading</td><td>Data</td></tr>'+
        	'</table>'+
    	'</div>';
	$("body").append(htmlstr);
	$("#"+name).draggable().resizable();
	this.dataTable = $("#"+name+"_data");
	this.setTable = function(data){
		$("#"+this.id+"_data").html(objectToTable(data));
	}.bind(this);
	
	window.setInterval(function(){updateFunction(this.setTable);}.bind(this),1000);
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
	

	
}

$(document).ready(function(){
	
	initMap();
	
	var counter = 1;
	
	window.setInterval(function(){
		var obj = {speed:counter,rpm:counter++ * 36,driver:"alive"};
		$("#car_data").html(
			JSONtoTable(JSON.stringify(obj))
		);
		obj = {status:"running", queued:(counter*93)%77,state:(counter%2 === 0)?"busy":"relaxed",queueBE:2,queueBK:5,queueVI:0,queueVO:100};
		$("#dcc_data").html(
			JSONtoTable(JSON.stringify(obj))
		);
	},10000);
	
	
	
	$(function() {
    	$( ".container" ).draggable().resizable();
  	});


});
