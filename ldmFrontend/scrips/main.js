

/**
 * 
 * @param {String} jsonString
 * @returm {String} html table body filles with values from json obj
 */
var JSONtoTable = function(jsonString){
	/** @type {JSON} */
	var jsonObj = JSON.parse(jsonString); 
	var str = ""; //"<table>";
	
	for (name in jsonObj){
		str += "<tr>";
		str += "<td>"+name+"</td>";
		str += "<td>"+jsonObj[name]+"</td>";
		str += "</tr>";
	}
	//str += "</table>";
	return str;
};


function queryLdmBackend(){
	$.post("http://localhost:1188/add_json",JSON.stringify({a:2,b:3}),function(data,status,xhr){
		console.log("data: "+data);
		console.log("status: "+status);
	});

}

function initMap(){
	//init map
	var map = L.map('mapContainer');

	// create the tile layer with correct attribution
	var osmUrl="map/openstreetmap/{z}/{x}/{y}.png";
	//online map 'http://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png';
	//marbel chache file:///home/jonh/.local/share/marble/maps/earth/openstreetmap/{z}/{x}/{y}.png
	
	
	var osmAttrib='Map data © <a href="http://openstreetmap.org">OpenStreetMap</a> contributors';
	var osm = new L.TileLayer(osmUrl, {minZoom: 1, maxZoom: 18, attribution: osmAttrib});		

	// start the map in South-East England
	map.setView(new L.LatLng(51.7315, 8.739),15);
	map.addLayer(osm);
	
}

//queryLdmBackend();

$(document).ready(function(){
	
	initMap();
	
	var counter = 1;
	
	window.setInterval(function(){
		var obj = {speed:counter,rpm:counter++ * 36,driver:"alive"};
		$("#car_data").html(
			JSONtoTable(JSON.stringify(obj))
		);
		var obj = {status:"running", queued:(counter*93)%77,state:(counter%2 == 0)?"busy":"relaxed",queueBE:2,queueBK:5,queueVI:0,queueVO:100};
		$("#dcc_data").html(
			JSONtoTable(JSON.stringify(obj))
		);
	},1000);
});