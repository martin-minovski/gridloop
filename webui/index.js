/***** index.js *****/
var express = require('express');
var app = express();
var http = require('http').Server(app);
var osc = require('osc-min');
var dgram = require('dgram');

// setup OSC
var OSC_SEND_PORT = 4368;
var udp = dgram.createSocket('udp4');

// start web server
http.listen(3000, function(){
	console.log('webpage active at URL 0.0.0.0:'+3000);
});
app.use(express.static('./'));

// open websocket
var io = require('socket.io')(http);
sockets = io.of('/');

// listen for websocket events
sockets.on('connection', function(socket){
	
	console.log('websocket connected!');
	
	socket.on('slider', data => {
		sendOSC({
			address: 'slider',
			args: [
				{
					type: 'integer',
					value: data.id
				},
				{
					type: 'float',
					value: data.value
				}
			]
		});
	});

    socket.on('piano', data => {
        sendOSC({
            address: 'piano',
            args: [
                {
                    type: 'integer',
                    value: data.note
                },
                {
                    type: 'integer',
                    value: data.state ? 1 : 0
                }
            ]
        });
    });
	
});

function sendOSC(message){
	var buffer = osc.toBuffer(message);
	udp.send(buffer, 0, buffer.length, OSC_SEND_PORT, '127.0.0.1', function(err) {
		if (err) console.log(err);
	});
}
