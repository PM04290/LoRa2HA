var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
var client;

function onLoad(event) {
	initWebSocket();
	$('#formconf').submit(function( event ) {
		event.preventDefault();
		var posting = $.post(event.currentTarget.action, $(this).serialize() );
		posting.done(function( data ) {
			$("#resultconf").fadeTo(100, 1);
			window.setTimeout(function() {$("#resultconf").fadeTo(500, 0)}, 2000);
			//console.log(data);
		});
	});
	$('#formwifi').submit(function( event ) {
		event.preventDefault();
		var posting = $.post(event.currentTarget.action, $(this).serialize() );
		posting.done(function( data ) {
			$("#resultwifi").fadeTo(100, 1);
			window.setTimeout(function() {$("#resultwifi").fadeTo(500, 0)}, 2000);
			//console.log(data);
		});
	});
	classdata["sensor"].forEach(function (arrayItem) {
		for (const property in arrayItem) {
			$("#classlist").append($('<option>', {value:property}));
		}
	});
}

function initWebSocket() {
	//console.log('Trying to open a WebSocket connection...');
	websocket = new WebSocket(gateway);
	websocket.onopen    = WSonOpen;
	websocket.onclose   = WSonClose;
	websocket.onmessage = WSonMessage;
}

function WSonOpen(event) {
	//console.log('Connection opened');
}

function WSonClose(event) {
	//console.log('Connection closed');
}

function WSonMessage(event) {
	var cmd = "";
	//console.log(event.data);
	JSON.parse(event.data, (key, value) => {
		//console.log(cmd + ":" + key+'='+value);
		if (key === "cmd") {
			cmd = value;
		} else
		if (key != "" && typeof value !== 'object') {
			if (cmd === "html") {
				$(key).html( value );
			} else
			if (cmd === "replace") {
				$(key).replaceWith( value );
			} else
			if (cmd === "log") {
				console.log( value );
			}
		} else {
			if (cmd === "loadselect") {
				loadSelect();
			}
		}
	});
}

function loadSelect() {
	$("input[name$='unit']").focus(function() {
		let arr = this.name.split("_");
		arr.pop();
		arr.push('class');
		let classname = arr.join('_');
		cval = $("input[name="+classname+"]").val();
		$("#unitlist").html('');
		classdata["sensor"].forEach(function (arrayItem) {
			for (const property in arrayItem) {
				if (cval === property) {
					arrayItem[property].forEach(function (unit) {
						$("#unitlist").append($('<option>', {value:unit}));
					});
				}
			}
		});
	});
	$("#dev_child_sensortype").on( "change", function() {
		let n = $(this).val();
		if (n < 2) {
			$("#sensor_param").show();
		} else {
			$("#sensor_param").hide();
			$("#dev_child_class").val("").change();
			$("#dev_child_unit").val("").change();
			$("#dev_child_expire").val(0).change();
			$("#dev_child_min").val("").change();
			$("#dev_child_max").val("").change();
		}
	});
}

function logpacket(elt) {
	websocket.send("logpacket;"+(elt.checked?"1":"0"));
}

function adddev(elt) {
	var nbdev = $("input[name$='address']").length;
	$("#conf_dev").append("<div id='newdev_"+nbdev+"'>device "+nbdev+"...</div>");
	websocket.send("adddev;"+nbdev);
}

function addchild(elt) {
	var arr = elt.id.split(/[_]+/);
	var nbchild = $("div[id^='conf_child_"+arr[1]+"']").length;
	$("<div id='conf_child_"+arr[1]+"_"+nbchild+"' class='row'>Loading...</div>").insertAfter("#conf_child_"+arr[1]+"_"+(nbchild-1));
	websocket.send("addchild;"+arr[1]+";"+nbchild);
}

const editHA = (event, d, c) => {
	$("#modal_d").val(d).change();
	$("#modal_c").val(c).change();
	$("#dev_child_sensortype").val($("input[name='dev_"+d+"_childs_"+c+"_sensortype']").val()).change();
	$("#dev_child_class").val($("input[name='dev_"+d+"_childs_"+c+"_class']").val()).change();
	$("#dev_child_unit").val($("input[name='dev_"+d+"_childs_"+c+"_unit']").val()).change();
	$("#dev_child_expire").val($("input[name='dev_"+d+"_childs_"+c+"_expire']").val()).change();
	$("#dev_child_min").val($("input[name='dev_"+d+"_childs_"+c+"_min']").val()).change();
	$("#dev_child_max").val($("input[name='dev_"+d+"_childs_"+c+"_max']").val()).change();
	toggleModal(event);
};

const confirmModal = (event) => {
	event.preventDefault();
	let d = $("#modal_d").val();
	let c = $("#modal_c").val();
	$("input[name='dev_"+d+"_childs_"+c+"_sensortype']").val($("#dev_child_sensortype").val());
	$("#li_dev_"+d+"_childs_"+c+"_sensortype").html($("#dev_child_sensortype option:selected").text());
	$("input[name='dev_"+d+"_childs_"+c+"_class']").val($("#dev_child_class").val());
	$("#li_dev_"+d+"_childs_"+c+"_class").html($("#dev_child_class").val());
	$("input[name='dev_"+d+"_childs_"+c+"_unit']").val($("#dev_child_unit").val());
	$("#li_dev_"+d+"_childs_"+c+"_unit").html($("#dev_child_unit").val());
	$("input[name='dev_"+d+"_childs_"+c+"_expire']").val($("#dev_child_expire").val());
	$("#li_dev_"+d+"_childs_"+c+"_expire").html($("#dev_child_expire").val());
	$("input[name='dev_"+d+"_childs_"+c+"_min']").val($("#dev_child_min").val());
	$("#li_dev_"+d+"_childs_"+c+"_min").html($("#dev_child_min").val());
	$("input[name='dev_"+d+"_childs_"+c+"_max']").val($("#dev_child_max").val());
	$("#li_dev_"+d+"_childs_"+c+"_max").html($("#dev_child_max").val());
	const modal = document.getElementById(event.currentTarget.getAttribute("data-target"));
	closeModal(modal);
};

var devicetype = ["Binary sensor", "Numeric sensor", "Switch", "Light", "Cover", "Fan", "HVac", "Select", "Trigger", "Custom", "Tag", "Text"];
var classdata = {
	"binary_sensor":["battery","battery_charging","carbon_monoxyde","cold","connectivity","door","garage_door","gas","heat","light","lock","moisture","motion","moving","occupency","plug","power","presense","problem","running","safety","smoke","sound","tamper","update","vibration","window"],
	"sensor":[
		{"apparent_power":["VA"]},
		{"aqi":[]},
		{"atmospheric_pressure":["cbar","bar","hPA","inHg","kPa","mbar","Pa","psi"]},
		{"battery":["%"]},
		{"carbon_dioxyde":["ppm"]},{"carbon_monoxyde":["ppm"]},
		{"current":["A","mA"]},
		{"data_rate":["bit/s","kbit/s","Mbit/s","Gbit/s","B/s","kB/s","MB/s","GB/s"]},
		{"data_size":["bit","kbit","Mbit","Gbit","B","kB","MB","GB","TB"]},
		{"date":[]},
		{"distance":["km","m","cm","mm","mi","yd","in"]},
		{"duraction":["d","h","min","sec"]},
		{"energy":["Wh","kWh","MWh"]},
		{"frequency":["Hz","kHz","MHz","GHz"]},
		{"gas":["m³","ft³","CCF"]},
		{"humidity":["%"]},
		{"illuminance":["lx"]},
		{"irradiance":["W/m²","BTU/(h.ft²)"]},
		{"moisture":["%"]},
		{"monetary":["€","$"]},
		{"nitrogen_dioxyde":["µg/m³"]},{"nitrogen_monoxyde":["µg/m³"]},{"nitrous_oxyde":["µg/m³"]},{"ozone":["µg/m³"]},
		{"pm1":["µg/m³"]},{"pm10":["µg/m³"]},{"pm25":["µg/m³"]},
		{"power_factor":["%"]},
		{"power":["W","kW"]},
		{"precipitation":["cm","in","mm"]},{"precipitation_intgensity":["in/d","in/h","mm/d","mm/h"]},
		{"pressure":["Pa","kPa","hPa","bar","cbar","mbar","mmHg","inHg","psi"]},
		{"reactive_power":["var"]},
		{"signal_strength":["dB","dBA"]},
		{"speed":["ft/s","in/d","in/h","km/h","kn","m/s","mph","mm/d"]},
		{"sulphur_dioxyde":["µg/m³"]},
		{"temperature":["°C","°F"]},
		{"timestamp":[]},
		{"volatile_organic_compounds":["µg/m³"]},
		{"voltage":["V","mV"]},
		{"volume":["L","mL","gal","fl.oz.","m³","ft³","CCF"]},
		{"water":["L","gal","m³","ft³","CCF"]},
		{"weight":["kg","g","mg","µg","oz","lb","st"]},
		{"wind_speed":["ft/s","km/h","kn","m/s","mph"]},
		]
};