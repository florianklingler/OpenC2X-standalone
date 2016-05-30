

function queryLdmBackend(){
	$.post("http://localhost:1188/add_json",JSON.stringify({a:2,b:3}),function(data,status,xhr){
		console.log("data: "+data);
		console.log("status: "+status);
	});

}

queryLdmBackend();

$(document).ready(function(){
	//init map
	var map = L.map('mapContainer');

	// create the tile layer with correct attribution
	var osmUrl='http://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png';
	var osmAttrib='Map data © <a href="http://openstreetmap.org">OpenStreetMap</a> contributors';
	var osm = new L.TileLayer(osmUrl, {minZoom: 12, maxZoom: 20, attribution: osmAttrib});		

	// start the map in South-East England
	map.setView(new L.LatLng(51.7315, 8.739),16);
	map.addLayer(osm);
});