//
// LoopGrid by Martin Minovski, 2018
//

var LOOPER_CHANNELS = 8;
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
var looperChannels = LOOPER_CHANNELS;
var faustWidgets = [];
var widgetIDs = [];
var instruments = {};
var saveRequested = false;
var activeChannel = 0;
var activeVariation = 0;
var zoomFactor = 1;
var zoomStep = 0.25;
var gridItems = [];
var recButton;
var shiftPressed = false;
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
    '#384658',
    '#8ec127',
    '#ff7373',
    '#caaac0',
    '#edd97c'
];
var nexusUIcolors = {
    // "accent": "#232b2b",
    // "fill": "#dc3d24",
    // "accent": "#dc3d24",
    "accent": "#dc3d24",
    "fill": "#232b2b",
    // "light": "#ffb745",
    "dark": "#000",
    "mediumLight": "#000",
    // "mediumDark": "#594d46"
};