
function getCurrentScale() {
    return zoomFactor;
}

function makeId() {
    var text = "";
    var possible = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

    for (var i = 0; i < 5; i++)
        text += possible.charAt(Math.floor(Math.random() * possible.length));

    return text;
}

function initWidgetsArray() {
    faustWidgets = [];
    widgetIDs = [];
    for (var i = 0; i < looperChannels; i++) {
        faustWidgets[i] = [];
    }
}

function updateFaustZone(zone, value) {
    socket.emit('ui', {
        address: 'updatezone',
        args: [
            {
                type: 'string',
                value: zone.toString()
            },
            {
                type: 'float',
                value: value
            }
        ]
    });
}

function faustSave() {
    saveRequested = true;
    var code = editor.getValue();
    socket.emit('ui', {
        address: 'writefaustcode',
        args: [
            {
                type: 'integer',
                value: editingChannel
            },
            {
                type: 'string',
                value: code
            }
        ]
    });
}

function faustOpen(channel, code) {
    $('#faust-channel-span').html(channel+1);
    $('#faust-editor-container').show();
    setBlur(1);
    editor.setValue(code, 1);
    isEditorOpen = true;
}

function faustClose() {
    $('#faust-editor-container').hide();
    setBlur(0);
    isEditorOpen = false;
}

function editDSP(channel) {
    editRequested = true;
    editingChannel = channel;
    socket.emit('ui', {
        address: 'getfaustcode',
        args: [
            {
                type: 'integer',
                value: channel
            }
        ]
    });
}

function styllize(object) {
    var colors = {
        "accent": "#232b2b",
        "fill": "#dc3d24",
        // "light": "#ffb745",
        "dark": "black",
        "mediumLight": "black",
        // "mediumDark": "#594d46"
    };
    for (var type in colors) {
        object.colorize(type, colors[type]);
    }
}

function setBlur(value) {
    var filterVal = 'blur(' + value + 'px)';
    $('.zoomViewport')
        .css('filter',filterVal)
        .css('webkitFilter',filterVal)
        .css('mozFilter',filterVal)
        .css('oFilter',filterVal)
        .css('msFilter',filterVal);
}
function activateChannel(channel, variation) {
    socket.emit('ui', {
        address: 'looperchannel',
        args: [
            {
                type: 'integer',
                value: channel
            },
            {
                type: 'integer',
                value: variation
            }
        ]
    });
    getChannelSummary();
}

function updateAllWidgets() {
    socket.emit('ui', {
        address: 'getwidgets',
        args: []
    });
}

function getInstruments() {
    socket.emit('ui', {
        address: 'getinstruments',
        args: []
    });
}

function fullscreen() {
    var isInFullScreen = (document.fullscreenElement && document.fullscreenElement !== null) ||
        (document.webkitFullscreenElement && document.webkitFullscreenElement !== null) ||
        (document.mozFullScreenElement && document.mozFullScreenElement !== null) ||
        (document.msFullscreenElement && document.msFullscreenElement !== null);

    var docElm = document.documentElement;
    if (!isInFullScreen) {
        if (docElm.requestFullscreen) {
            docElm.requestFullscreen();
        } else if (docElm.mozRequestFullScreen) {
            docElm.mozRequestFullScreen();
        } else if (docElm.webkitRequestFullScreen) {
            docElm.webkitRequestFullScreen();
        } else if (docElm.msRequestFullscreen) {
            docElm.msRequestFullscreen();
        }
    } else {
        if (document.exitFullscreen) {
            document.exitFullscreen();
        } else if (document.webkitExitFullscreen) {
            document.webkitExitFullscreen();
        } else if (document.mozCancelFullScreen) {
            document.mozCancelFullScreen();
        } else if (document.msExitFullscreen) {
            document.msExitFullscreen();
        }
    }
}

function getClipSummary() {
    socket.emit('ui', {
        address: 'getclipsummary',
        args: []
    });
}

function getChannelSummary() {
    socket.emit('ui', {
        address: 'getchannelsummary',
        args: []
    });
}

function getActive() {
    socket.emit('ui', {
        address: 'getactive',
        args: []
    });
}

function clearClips() {
    socket.emit('ui', {
        address: 'clearchannel',
        args: [
            {
                type: 'integer',
                value: activeChannel
            },
            {
                type: 'integer',
                value: activeVariation
            }
        ]
    });
}

function loopGroupCtrl(variation) {
    socket.emit('ui', {
        address: 'loopergroupvariation',
        args: [
            {
                type: 'integer',
                value: variation
            }
        ]
    });
    getChannelSummary();
}

function zoomIn() {
    zoomFactor += zoomStep;
    zoomControl(zoomFactor);
}

function zoomOut() {
    zoomFactor -= zoomStep;
    zoomControl(zoomFactor);
}

function zoomControl(factor) {
    $('.grid')
        .css('zoom', factor.toString())
        .css('transform', '-moz-scale(' + factor.toString() + ')');
}
