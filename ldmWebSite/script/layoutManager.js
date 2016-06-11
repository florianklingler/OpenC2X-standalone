//each container class div needs and unique id

/**
 * saves to current layout to localstorage on client side
 */
function saveLayout(){
	var containers = $(".container");
	var containerIDs = [];
	
	for (var i = 0; i< containers.length;i++){
		var con = containers[i];
		containerIDs.push(con.id);
		var config = {
			offset: $("#"+con.id).offset(),
			height: $("#"+con.id).height(),
			width: $("#"+con.id).width()
		}
		localStorage.setItem(con.id, JSON.stringify(config));
	}
	localStorage.setItem("containerIDs",JSON.stringify(containerIDs));
}


/**
 * loads and applys layout configs peviously saved via saveLayout()
 */
function loadLayout(){
	var containerIDs = localStorage.getItem("containerIDs");
	if(containerIDs == null){
		console.log("No layout saved.");
		return;
	}
	containerIDs = JSON.parse(containerIDs);
	
	containerIDs.forEach(function(id){
		var config = localStorage.getItem(id);
		if(config == null){
			console.log("No layout information for "+id+" present.");
			return;
		}
		config  = JSON.parse(config);
		$("#"+id).offset(config.offset),
		$("#"+id).height(config.height),
		$("#"+id).width(config.width)
		
	})
}


function lockLayout(){
	$(".container").draggable("disable").resizable("disable");
}

function unlockLayout(){
	$(".container").draggable("enable").resizable("enable");
}