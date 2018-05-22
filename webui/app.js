/**
 * Websockets / OSC glue program
 * Created by Martin on 4/28/18.
 */


var express = require('express');
var app = express();
var http = require('http').Server(app);

var HTTP_PORT = 3000;

var os = require('os');

// Start the web server
http.listen(HTTP_PORT, function(){
    printIps();
});
app.use(express.static('./'));

// Open websocket
var io = require('socket.io')(http);
sockets = io.of('/');

// Listen for websocket events
sockets.on('connection', function(socket) {
    var address = socket.handshake.address;
    console.log('Websocket connected - ' + address);
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
    console.log("Listening for OSC on port " + address.port);
});

udpInbound.on('message', function (data) {
    var oscMsg = osc.fromBuffer(data);
    sockets.emit('cppinput', oscMsg);
});

/**
 * Function to print active IP interfaces
 */
function printIps() {
    var ifaces = os.networkInterfaces();

    Object.keys(ifaces).forEach(function (ifname) {
        var alias = 0;

        ifaces[ifname].forEach(function (iface) {
            if ('IPv4' !== iface.family || iface.internal !== false) {
                // skip over internal (i.e. 127.0.0.1) and non-ipv4 addresses
                return;
            }

            if (alias >= 1) {
                // this single interface has multiple ipv4 addresses
                console.log('Web app active on ' + iface.address + ':' + HTTP_PORT + ' (' + ifname + ':' + alias + ')');
            } else {
                // this interface has only one ipv4 adress
                console.log('Web app active on ' + iface.address + ':' + HTTP_PORT + ' (' + ifname + ')');
            }
            ++alias;
        });
    });
}