
var path = require('path');
var fs = require('fs');

var plotly = require('plotly')('rickkas7', 'xxxx');

var trace1 = {
		x: [],
		y: [],
		type: "scatter",
		name: "Adafruit"
};
var trace2 = {
		x: [],
		y: [],
		type: "scatter",
		name: "TinyGPS++"
};

var data = fs.readFileSync( path.join(__dirname, 't6.txt'), 'utf8');


var dataArray = data.split("\n");
for(var ii = 0; ii < dataArray.length; ii++) {
	var entries = dataArray[ii].trim().split(',');
	if (entries.length == 4) {
		trace1.x.push(parseFloat(entries[0]));
		trace1.y.push(parseFloat(entries[1]));
		trace2.x.push(parseFloat(entries[2]));
		trace2.y.push(parseFloat(entries[3]));
	}
}

var data = [trace1, trace2];
var graphOptions = {filename: "gps-error", fileopt: "overwrite"};
plotly.plot(data, graphOptions, function (err, msg) {
	console.log(msg);
});

