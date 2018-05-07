/**
 * Websockets / OSC glue program
 * Created by Martin on 4/28/18.
 */


var express = require('express');
var app = express();
var http = require('http').Server(app);

var HTTP_PORT = 3000;

// Start the web server

http.listen(HTTP_PORT, function(){
	console.log('Webpage active at URL localhost:' + HTTP_PORT);
});
app.use(express.static('./'));

// Open websocket
var io = require('socket.io')(http);
sockets = io.of('/');

// Listen for websocket events
sockets.on('connection', function(socket){
    console.log('Websocket connected!');
    socket.on('ui', data => {
        sendOSC(data);
    });
});

/**
 * Setup Node -> C++ OSC comms
  */
var OSC_SEND_PORT = 4368;
var osc = require('osc-min');
var dgram = require('dgram');
var udp = dgram.createSocket('udp4');

function sendOSC(message){
	var buffer = osc.toBuffer(message);
	udp.send(buffer, 0, buffer.length, OSC_SEND_PORT, '127.0.0.1', function(err) {
		if (err) console.log(err);
	});
}

/**
 * Setup C++ -> node OSC comms
 */
var OSC_RECEIVE_PORT = 10295;
var udpInbound = dgram.createSocket('udp4');
udpInbound.bind(OSC_RECEIVE_PORT);

udpInbound.on('listening', function() {
    var address = udpInbound.address();
    console.log("Listening on: " + address.address + ":" + address.port);
});

udpInbound.on('message', function (data) {
    var oscMsg = osc.fromBuffer(data);
    sockets.emit('cppinput', oscMsg);
});