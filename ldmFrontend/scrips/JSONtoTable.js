
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
	console.log(jsonObj);
	return str;
};

/*
$(document).ready(function(){

	var obj = {a:4,b:6};
	var stringy = JSON.stringify(obj);

	$("#result").html(
		stringy+"<p>"+JSONtoTable(stringy)
	);
});*/