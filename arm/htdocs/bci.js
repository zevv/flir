
var batt_min = 1;
var batt_max = 64;

function bci(cmd, data, fn) {

	$.ajax({
		url: "/bci?cmd=" + cmd,
		dataType: 'json',
		data: data,
		success: function(d) { fn(true, d); },
		error: function() { fn(false, "error"); }
	});
}

function decimals(v, n) {
	return Number(v).toFixed(n);
}

$(document).ready(function() {

	function poll_accum_data()
	{
		bci("bci_get_accum_data", null, function(ok, d) {
			if(ok) {
				var data = d.data;
				var h = "";

				$("#accum-soc").html(decimals(data.soc, 0) + "%");
				$("#accum-eta").html("Time remaining: <b>17d 8h 4m</b>")

				h += "" + decimals(data.U, 2) + " V / ";
				h += "" + decimals(data.I, 2) + " A / ";
				h += "" + decimals(data.T_batt, 1) + " °C";
				$("#accum-details").html(h)
			} else {
				$("#accum-soc").html("")
				$("#accum-eta").html("")
				$("#accum-details").html("Error connecting to the BCI")
			}
			setTimeout(poll_accum_data, 500);
		});
	}

	function get_battery_data(b, batt_id)
	{
		bci("batt_get_data", { "node_id": batt_id }, function(ok, bd) {
			var h = "<b>" + batt_id + "</b><br>";
			if(ok) {
				var data = bd.data;
				h += "" + decimals(data.U, 2) + "V<br>";
				h += "" + data.soc + "%<br>";
			} else {
				h += "error";
			}

			b.html(h);
		});
	}

	function poll_batteries()
	{
		bci("bci_get_batt_state", null, function(ok, d) {
			if(ok) {

				var i;
				for(i=batt_min; i<=batt_max; i++) {

					var b = $(".battery-" + i);

					if(b) {

						var state = d.data[i];
						b.attr("data-state", state);

						if(state == "available") {
							get_battery_data(b, i);
						} else {
							b.html("<b>" + i + "</b><br><font size=0.7em>" + state + "</font>");
						}

					}
				}
			}
			setTimeout(poll_batteries, 500);
		});
	}

	var h = "";
	for(i=batt_min; i<=batt_max; i+=2) {
		h += "<div class='batterygroup'>\n";
		for(j=0; j<2; j++) {
			h += " <div class='battery battery-" + (i+j) + "'></div>\n"
		}
		h += "</div>\n";
	}
	$("#batteries").html(h)

	poll_batteries();
	poll_accum_data();

});

// vi: ft=javascript ts=3 sw=3
