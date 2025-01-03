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
			window.setTimeout(function() {alert("The HUB must be reset for the configuration to be active")}, 100);
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
	$("input:radio[name='uplsource']").on( "change", function() {
		$("#upllist").html("<option disabled>Loading the list...</option>");
		$("#upldel").prop("disabled",$(this).val() === "1");
		websocket.send("uplmodule;open;"+$(this).val());
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
		//console.log(cmd + ":" + key + '=' + value);
		if (key === "cmd") {
			cmd = value;
		} else
		if (key !== "" && typeof value !== 'object') {
			if (cmd === "html") {
				$(key).html( value );
			} else
			if (cmd === "replace") {
				$(key).replaceWith( value );
			} else
			if (cmd === "log") {
				console.log( value );
			}
			if (cmd === "uplmsg") {
				$("#resultupload").html(value);
				$("#resultupload").fadeTo(100, 1);
				window.setTimeout(function() {$("#resultupload").fadeTo(500, 0)}, 2000);
			}
			if (cmd === "blocnotify") {
				$('#conf_dev > mark:first').remove();
				if ($("#"+key).length === 0)
				{
					$("#conf_dev").append( value );
					loadSelect();
				}
			}
			if (cmd === "devnotify") {
				if ($("#"+key).length)
					$("#"+key).html(value);
				else {
					let arr = key.split(/[_]+/);
					let d = parseInt(arr[2]);
					$("<div id='"+key+"' class='row'>"+value+"<div>").insertAfter("#header_"+(d));
				}
				loadSelect();
			}
			if (cmd === "childnotify") {
				if ($("#"+key).length)
				$("#"+key).html(value);
				else {
					let arr = key.split(/[_]+/);
					let d = parseInt(arr[2]);
					$("<div id='"+key+"' class='row'>"+value+"<div>").insertBefore("#footer_"+(d));
				}
				loadSelect();
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
	$("input[name$='_name']").on( "change", function() {
		let n = $(this).val();
		if (!n) {
			$(this).attr("aria-invalid", true);
			} else {
			$(this).removeAttr("aria-invalid");
		}
	});
	$("input[name$='_label']").on( "change", function() {
		let n = $(this).val();
		if (!n) {
			$(this).attr("aria-invalid", true);
			} else {
			$(this).removeAttr("aria-invalid");
		}
	});
	$("#floatinfo").fadeTo(100, $("#selpairing").attr('checked')?1:0);
}

function logpacket(elt) {
	websocket.send("logpacket;"+(elt.checked?"1":"0"));
}

function changeAdr(d) {
	let newadr = $("input[id='newadr_"+d+"']").val();
	if (newadr === "") {
		alert("Address required");
		return;
	}
	$("#conf_dev").children().each(function () {
		let n = $(this).attr('id').split('_')[1];
		let adr = $("input[name='dev_"+d+"_address']").val();
        if (adr == newadr) {
			alert("This address is already used");
			return;
		}
	});
	websocket.send("newadr;"+d+";"+newadr);
}

function endPairing(d) {
	websocket.send("endpairing;"+d);
}

function uplmodule(event) {
	toggleModal(event);
	$("#upllist").html("<option disabled>Loading the list...</option>");
	websocket.send("uplmodule;open;"+$("input[name='uplsource']:checked").val());
}

function uplsend(event) {
	let file = $("#upllist").val();
	if (file) {
		let n = $("input[name='uplsource']:checked").val();
		$("#resultupload").html("<progress/>");
		$("#resultupload").fadeTo(10, 1);
		websocket.send("uplmodule;send;" + n + ";" + file);
	} else {
		$("#resultupload").html("You must select a firmware");
		$("#resultupload").fadeTo(100, 1);
		window.setTimeout(function() {$("#resultupload").fadeTo(500, 0)}, 2000);
	}
}

function upldel(event) {
	let file = $("#upllist").val();
	if (file) {
		websocket.send("uplmodule;del;" + file);
	} else {
		$("#resultupload").html("You must select a firmware");
		$("#resultupload").fadeTo(100, 1);
		window.setTimeout(function() {$("#resultupload").fadeTo(500, 0)}, 2000);
	}
}

function pairing(elt) {
	$("#floatinfo").fadeTo(100, elt.checked?1:0);
	websocket.send("pairing;"+(elt.checked?"1":"0"));
}

const editHA = (event, d, c) => {
	$("#modal_d").val(d).change();
	$("#modal_c").val(c).change();
	$("#dev_child_class").val($("input[name='dev_"+d+"_childs_"+c+"_class']").val()).change();
	$("#dev_child_category").val($("input[name='dev_"+d+"_childs_"+c+"_category']").val()).change();
	$("#dev_child_unit").val($("input[name='dev_"+d+"_childs_"+c+"_unit']").val()).change();
	$("#dev_child_expire").val($("input[name='dev_"+d+"_childs_"+c+"_expire']").val()).change();
	$("#dev_child_min").val($("input[name='dev_"+d+"_childs_"+c+"_min']").val()).change();
	$("#dev_child_max").val($("input[name='dev_"+d+"_childs_"+c+"_max']").val()).change();
	$("#dev_child_coefa").val($("input[name='dev_"+d+"_childs_"+c+"_coefa']").val()).change();
	$("#dev_child_coefb").val($("input[name='dev_"+d+"_childs_"+c+"_coefb']").val()).change();
	
	let n = $("input[name='dev_"+d+"_childs_"+c+"_sensortype").val();
	if (n <= 1) {
		$("#ha_class").show();
		$("#classlist").html('');
		$("#ha_category").show();
		$("#dev_child_category option[value='1']").prop("disabled",false);
		if (n == 0) {
			$("#ha_param").hide();
			$("#dev_child_category option[value='1']").prop("disabled",true);
			classdata["binary_sensor"].forEach(function (arrayItem) {
				$("#classlist").append($('<option>', {value:arrayItem}));
			});
		} else {
			$("#ha_param").show();
			$("#dev_child_category option[value='1']").prop("disabled",true);
			classdata["sensor"].forEach(function (arrayItem) {
				for (const property in arrayItem) {
					$("#classlist").append($('<option>', {value:property}));
				}
			});
		}
	} else if (n == 7 || n == 12) {
		$("#ha_class").hide();
		$("#ha_category").show();
		$("#ha_param").hide();
	} else {
		$("#ha_class").hide();
		$("#ha_category").hide();
		$("#ha_param").hide();
		$("#dev_child_class").val("").change();
		$("#dev_child_unit").val("").change();
		$("#dev_child_expire").val("").change();
		$("#dev_child_min").val("").change();
		$("#dev_child_max").val("").change();
		$("#dev_child_coefa").val("").change();
		$("#dev_child_coefb").val("").change();
	}
	
	toggleModal(event);
};

const confirmModal = (event) => {
	event.preventDefault();
	let d = $("#modal_d").val();
	let c = $("#modal_c").val();
	$("input[name='dev_"+d+"_childs_"+c+"_class']").val($("#dev_child_class").val());
	$("input[name='dev_"+d+"_childs_"+c+"_category']").val($("#dev_child_category").val());
	$("input[name='dev_"+d+"_childs_"+c+"_unit']").val($("#dev_child_unit").val());
	$("input[name='dev_"+d+"_childs_"+c+"_expire']").val($("#dev_child_expire").val());
	$("input[name='dev_"+d+"_childs_"+c+"_min']").val($("#dev_child_min").val());
	$("input[name='dev_"+d+"_childs_"+c+"_max']").val($("#dev_child_max").val());
	$("input[name='dev_"+d+"_childs_"+c+"_coefa']").val($("#dev_child_coefa").val());
	$("input[name='dev_"+d+"_childs_"+c+"_coefb']").val($("#dev_child_coefb").val());
	let defc_ha = "";
	let st = $("input[name='dev_"+d+"_childs_"+c+"_sensortype").val()*1;
	let cat = $("#dev_child_category").val()*1;
	if (cat == 1) {
		defc_ha += "&#9874;";
	} else if (cat == 2) {
		defc_ha += "&#128208;";
	} else {
		defc_ha += "&#128065;";
	}
	if (st == 0) {
		defc_ha += "<dfn>"+$("#dev_child_class").val() + "</dfn>";
	}
	if (st == 1) {
		defc_ha += "<dfn>"+$("#dev_child_class").val()+"</dfn><dfn>"+$("#dev_child_unit").val()+"</dfn>";
		if ($("#dev_child_min").val() != "" || $("#dev_child_max").val() != "" || $("#dev_child_coefa").val()*1.0 != 1.0 || $("#dev_child_coefb").val()*1.0 != 0.0)
			defc_ha += "&#9850;";
	}
	$("#desc_dev_"+d+"_childs_"+c).html(defc_ha);
	const modal = document.getElementById(event.currentTarget.getAttribute("data-target"));
	closeModal(modal);
};

var classdata = {
	"binary_sensor":["battery","battery_charging","carbon_monoxyde","cold","connectivity","door","garage_door","gas","heat","light","lock","moisture","motion","moving","occupency","opening","plug","power","presense","problem","running","safety","smoke","sound","tamper","update","vibration","window"],
	"sensor":[
	{"apparent_power":["VA"]},
	{"aqi":[]},
	{"atmospheric_pressure":["cbar","bar","hPA","inHg","kPa","mbar","Pa","psi"]},
	{"battery":["%"]},
	{"carbon_dioxyde":["ppm"]},
	{"carbon_monoxyde":["ppm"]},
	{"current":["A","mA"]},
	{"data_rate":["bit/s","kbit/s","Mbit/s","Gbit/s","B/s","kB/s","MB/s","GB/s"]},
	{"data_size":["bit","kbit","Mbit","Gbit","B","kB","MB","GB","TB"]},
	{"date":[]},
	{"distance":["km","m","cm","mm","mi","yd","in"]},
	{"duration":["d","h","min","sec"]},
	{"energy":["Wh","kWh","MWh"]},
	{"energy_storage":["Wh","kWh","MWh"]},
	{"frequency":["Hz","kHz","MHz","GHz"]},
	{"gas":["m³","ft³","CCF"]},
	{"humidity":["%"]},
	{"illuminance":["lx"]},
	{"irradiance":["W/m²","BTU/(h.ft²)"]},
	{"moisture":["%"]},
	{"monetary":["€","$"]},
	{"nitrogen_dioxyde":["µg/m³"]},
	{"nitrogen_monoxyde":["µg/m³"]},
	{"nitrous_oxyde":["µg/m³"]},
	{"ozone":["µg/m³"]},
	{"ph":[""]},
	{"pm1":["µg/m³"]},{"pm10":["µg/m³"]},{"pm25":["µg/m³"]},
	{"power":["W","kW"]},
	{"power_factor":["%"]},
	{"precipitation":["cm","in","mm"]},
	{"precipitation_intensity":["in/d","in/h","mm/d","mm/h"]},
	{"pressure":["Pa","kPa","hPa","bar","cbar","mbar","mmHg","inHg","psi"]},
	{"reactive_power":["var"]},
	{"signal_strength":["dB","dBm"]},
	{"sound_pressure":["dB","dBA"]},
	{"speed":["ft/s","in/d","in/h","km/h","kn","m/s","mph","mm/d"]},
	{"sulphur_dioxyde":["µg/m³"]},
	{"temperature":["°C","°F"]},
	{"timestamp":[]},
	{"volatile_organic_compounds":["µg/m³"]},
	{"volatile_organic_compounds_parts":["ppm","ppb"]},
	{"voltage":["V","mV"]},
	{"volume":["L","mL","gal","fl.oz.","m³","ft³","CCF"]},
	{"volume_flow_rate":["m³/h", "ft³/min", "L/min", "gal/min"]},
	{"volume_storage":["L","mL","gal","fl.oz.","m³","ft³","CCF"]},
	{"water":["L","gal","m³","ft³","CCF"]},
	{"weight":["kg","g","mg","µg","oz","lb","st"]},
	{"wind_speed":["ft/s","km/h","kn","m/s","mph"]},
]
};