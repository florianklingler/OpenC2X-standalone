/**
 * utility functions which have no dependencies on other .js files
 */


/**
 * brightens the hex color by percent
 * copied from http://stackoverflow.com/questions/6443990/javascript-calculate-brighter-colour
 * @param hex
 * @param percent
 * @returns {String}
 */
function increase_brightness(hex, percent){
    // strip the leading # if it's there
    hex = hex.replace(/^\s*#|\s*$/g, '');

    // convert 3 char codes --> 6, e.g. `E0F` --> `EE00FF`
    if(hex.length == 3){
        hex = hex.replace(/(.)/g, '$1$1');
    }

    var r = parseInt(hex.substr(0, 2), 16),
        g = parseInt(hex.substr(2, 2), 16),
        b = parseInt(hex.substr(4, 2), 16);

    return '#' +
       ((0|(1<<8) + r + (256 - r) * percent / 100).toString(16)).substr(1) +
       ((0|(1<<8) + g + (256 - g) * percent / 100).toString(16)).substr(1) +
       ((0|(1<<8) + b + (256 - b) * percent / 100).toString(16)).substr(1);
}

/**
 * converts a Object to a html table body
 * @param {Object} Object
 * @returm {String} html table body filles with values from obj
 */
function objectToTable(obj){
	var str = ""; //"<table>";
	
	for (var name in obj){
		str += "<tr>";
		str += "<td>"+name+"</td>";
		if (name == "gps" | name == "obd2" 
			| name.includes("Cat0")| name.includes("Cat1")| name.includes("Cat2")| name.includes("Cat3") /*categories of dccInfo*/){
			str += "<td><table>" + objectToTable(obj[name]) + "</table></td>";
		} else {
			if (name.includes("time") | name.includes("Time") | name.includes("delta")){//.includes is case sensitive
				str += "<td>";
				var date = new Date(Number(String(obj[name]).slice(0,-6)));//need to cut last 6 values cause Date() is not that precise
				str += date.getHours()+":"+date.getMinutes()+":"+date.getSeconds()+":"+date.getMilliseconds();
				str +="</td>";
			} else {
				str += "<td>"+obj[name]+"</td>";
			}
		}
		str += "</tr>";
	}
	//str += "</table>";
	return str;
}

/**
 * converts a JSON string to a html table body
 * @param {String} jsonString
 * @returm {String} html table body filles with values from json obj
 */
function JSONtoTable(jsonString){
	/** @type {JSON} */
	var jsonObj = JSON.parse(jsonString); 
	return objectToTable(jsonObj);
}
