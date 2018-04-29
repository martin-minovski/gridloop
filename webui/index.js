

var express = require('express');
var app = express();
var http = require('http').Server(app);


// start web server
http.listen(3000, function(){
	console.log('webpage active at URL 0.0.0.0:'+3000);
});
app.use(express.static('./'));


// setup OSC
var osc = require('osc-min');
var dgram = require('dgram');
var OSC_SEND_PORT = 4368;
var udp = dgram.createSocket('udp4');
// open websocket
var io = require('socket.io')(http);
sockets = io.of('/');

// listen for websocket events
sockets.on('connection', function(socket){
	
	console.log('websocket connected!');

	socket.on('ui', data => {
		sendOSC(data);
    });
	
});

function sendOSC(message){
	var buffer = osc.toBuffer(message);
	udp.send(buffer, 0, buffer.length, OSC_SEND_PORT, '127.0.0.1', function(err) {
		if (err) console.log(err);
	});
}
