var socket = io('/');
var editor = ace.edit("faust-editor");
var editingChannel = 0;
var editRequested = false;
var isEditorOpen = false;
var octaveShift = 0;
var volumeSliders = [];
var soloButtons = [];
var preventSend = false;
var midiRec = false;
var LOOPER_CHANNELS = 8;
var looperChannels = LOOPER_CHANNELS;
var faustWidgets = [];
var widgetIDs = [];
var instruments = {};
var saveRequested = false;
var activeChannel = 0;
var activeVariation = 0;
var zoomFactor = 1;
var zoomStep = 0.075;
var gridItems = [];
for (var i = 0; i < 8; i++) {
    gridItems[i] = [];
    for (var j = 0; j < 4; j++) {
        gridItems[i][j] = {
            element: undefined
        };
    }
}
var channelColors = [
    '#00aedb',
    '#a200ff',
    '#f47835',
    '#d41243',
    '#8ec127',
    '#ff7373',
    '#caaac0',
    '#edd97c'
];
