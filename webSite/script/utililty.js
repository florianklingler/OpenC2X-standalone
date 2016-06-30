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
	
	/**
	 * shotens #data if its a decimal number to 6 numbers after the dot. else just returns #data
	 */
	var shortenDecimalNumber = function(data){
		if (typeof data == "number"){
			return Math.round(data*1000000)/1000000;
		} else {
			return data;
		}
	};
	
	for (var name in obj){
		str += "<tr>";
		str += "<td>"+name+"</td>";
		if (typeof obj[name] == "object"){
			str += "<td><table>" + objectToTable(obj[name]) + "</table></td>";
		} else {
			if (name.includes("time") | name.includes("Time")){//.includes is case sensitive
				str += "<td>";
				var date = new Date(Number(String(obj[name]).slice(0,-6)));//need to cut last 6 values cause Date() is not that precise
				/** adds upto 1 leading zero*/
				var pad10 = function(n) {
				    return (n < 10) ? ("0" + n) : n; 
				}
				/** adds up to 2 leading zeros*/
				var pad100 = function(n) {
				    return (n < 10) ? ("00" + n) : 
				    	((n<100) ? "0" +n : n);
				}
				str += pad10(date.getHours())+":"+pad10(date.getMinutes())+":"+
					pad10(date.getSeconds())+":"+pad100(date.getMilliseconds());
				str +="</td>";
			} else {
				str += "<td>"+shortenDecimalNumber(obj[name])+"</td>";
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
