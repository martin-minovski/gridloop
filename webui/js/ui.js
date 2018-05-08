var editor = ace.edit("faust-editor");
editor.setFontSize(20);
var editingChannel = 0;
function editDSP(channel) {
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

var LOOPER_CHANNELS = 8;
var looperChannels = LOOPER_CHANNELS;
var faustWidgets = [];
var widgetIDs = [];

function initWidgetsArray() {
    faustWidgets = [];
    widgetIDs = [];
    for (var i = 0; i < looperChannels; i++) {
        faustWidgets[i] = [];
    }
}
initWidgetsArray();

var viewport = $('#mainContainer');
for (var i = 0; i < looperChannels; i++) {
    viewport.append('<div class="grid-item-channel grid-item-padded zoomTarget">' +
        '<div class="title"><div style="font-size: 10px; font-weight: 600">Channel ' + (i+1) + ' ' +
        '<i class="fa fa-pencil-alt" onclick="editDSP(' + i + ')" style="cursor: pointer"></i>' +
        '</div></div>' +
        '<div id="widget-container-' + i + '"></div>' +
        '</div>');
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

function faustClose() {
    $('#faust-editor-container').hide();
}

var socket = io('/');
socket.on('cppinput', function (data) {
    if (data.address === 'faust_ack') {
        updateAllWidgets();
        alert("Compiled with no errors!");
    }
    if (data.address === 'faust_code') {
        var channel = data.args[0].value;
        var code = data.args[1].value;
        $('#faust-channel-span').html(channel+1);
        $('#faust-editor-container').show();
        editor.setValue(code, 1);
    }
    if (data.address === 'faust_error') {
        updateAllWidgets();
        alert(data.args[0].value);
    }
    if (data.address === 'json_update') {
        initWidgetsArray();
        var jsonString = data.args[0].value;
        var widgets = JSON.parse(jsonString);
        widgets.forEach(function(widget) {
            faustWidgets[widget.looperChannel][widget.name] = [];
        });
        widgets.forEach(function(widget) {
            faustWidgets[widget.looperChannel][widget.name][widget.axis] = widget;

        });

        console.log("Widgets array: ", faustWidgets);

        for (var i = 0; i < looperChannels; i++) {
            var widgetContainer = $('#widget-container-' + i);
            widgetContainer.html('');
            for (var j in faustWidgets[i]) {
                var widget = faustWidgets[i][j];
                var idSafeName = j.replace(/ /g, "_").replace(/"/g, "").replace(/'/g, "").replace(/\(|\)/g, "");
                var widgetID = 'widget_' + i + '_' + idSafeName;
                widgetIDs['#' + widgetID] = widget;

                var axisX = widget[0];
                var axisY = widget[1];
                var axisZ = widget[2];

                var widgetDiv = $('<div style="font-size: 10px;">' + j + '<br /><div class="widget-div" id="' + widgetID + '"></div></div>');
                widgetContainer.append(widgetDiv);


                if (axisX.type === 'slider') {
                    var nexusUiWidget = new Nexus.Slider('#' + widgetID, {
                        'size': [140,20],
                        'mode': 'relative',
                        'min': axisX.min,
                        'max': axisX.max,
                        'step': axisX.step,
                        'value': axisX.value
                    });
                    nexusUiWidget.on('change',function(value) {
                        var widget = widgetIDs[this.settings.target];
                        updateFaustZone(widget[0].zone, value);
                    });
                }
                else if (axisX.type === 'xypad') {
                    $('#' + widgetID).addClass('xy-pad-container');
                    var nexusUiWidget = new Nexus.Position('#' + widgetID, {
                        'size': [140,140],
                        'mode': 'relative',
                        'x': axisX.value,
                        'minX': axisX.min,
                        'maxX': axisX.max,
                        'stepX': axisX.step,
                        'y': axisY.value,
                        'minY': axisY.min,
                        'maxY': axisY.max,
                        'stepY': axisY.step
                    });
                    nexusUiWidget.on('change',function(value) {
                        var widget = widgetIDs[this.settings.target];
                        updateFaustZone(widget[0].zone, value.x);
                        updateFaustZone(widget[1].zone, value.y);
                    });
                }

            }
        }
    }
});

var octaveShift = 0;

$(document).ready(function() {

    var piano = new Nexus.Piano('#piano', {
        'size': [300, 125],
        'mode': 'button',  // 'button', 'toggle', or 'impulse'
        'lowNote': 48,
        'highNote': 73
    });

    piano.on('change',function(data) {
        socket.emit('ui', {
            address: 'piano',
            args: [
                {
                    type: 'integer',
                    value: data.note
                },
                {
                    type: 'integer',
                    value: data.state ? 1 : 0
                },
                {
                    type: 'integer',
                    value: octaveShift
                }
            ]
        });
    });

    var recButton = new Nexus.Button('#looper-rec',{
        'size': [80, 80],
        'mode': 'toggle',  // 'button', 'toggle', or 'impulse'
        'state': false
    });

    recButton.on('change',function(v) {
        var recDirect = !$('#pianoZoomTarget').hasClass('selectedZoomTarget');
        socket.emit('ui', {
            address: 'rec',
            args: [
                {
                    type: 'integer',
                    value: v ? 1 : 0
                },
                {
                    type: 'integer',
                    value: recDirect ? 1 : 0
                }
            ]
        });
    });

    $('.oct-control').click(function() {
        var meta = $(this).attr('meta');
        if (meta === '+') {
            octaveShift++;
        }
        if (meta === '-') {
            octaveShift--;
        }
    });

    var chArray = [];
    for (var i = 0; i < looperChannels; i++) {
        chArray.push((i+1).toString());
        var channelArea = $('<div style="display: inline-block"></div>');
        channelArea.append($('<div slider="channelvolume" meta="' + i + '" sizeX="30" sizeY="115" style="margin-left: 6px;"></div>'));
        channelArea.append($('<div style="height:5px;position:relative;"><div style="position:absolute;margin-top:12px;margin-left:10px;pointer-events:none;color:#bbb">S</div></div>'));
        channelArea.append($('<div solochannel="' + i + '"></div>'));
        $('#looper-channel-holder').append(channelArea);
    }
    var chSelect = new Nexus.Select('#looper-ch',{
        'size': [100,30],
        'options': chArray
    });
    chSelect.on('change',function(data) {
        var index = data.index;
        var value = data.value;
        socket.emit('ui', {
            address: 'looperchannel',
            args: [
                {
                    type: 'integer',
                    value: index
                }
            ]
        });
    });

    $('[slider]').each(function() {
        var newId = makeId();
        $(this).attr('id', newId);
        var sizeX = $(this).attr('sizeX');
        var sizeY = $(this).attr('sizeY');
        var min = $(this).attr('min');
        var max = $(this).attr('max');
        var step = $(this).attr('step');
        var value = $(this).attr('value');
        var meta = $(this).attr('meta');

        var gainSlider = new Nexus.Slider('#' + newId ,{
            'size': [parseInt(sizeX), parseInt(sizeY)],
            'mode': 'absolute',  // 'relative' or 'absolute'
            'min': min ? parseInt(min) : 0,
            'max': max ? parseInt(max) : 1,
            'step': step ? parseInt(step) : 0,
            'value': value ? parseInt(value) : 1
        });

        gainSlider.on('change',function(val) {
            socket.emit('ui', {
                address: 'slider',
                args: [
                    {
                        type: 'string',
                        value: $(this.parent).attr('slider')
                    },
                    {
                        type: 'float',
                        value: val
                    },
                    {
                        type: 'integer',
                        value: meta ? parseInt(meta) : 0
                    }
                ]
            });
        });
    });

    $('[instrument]').each(function() {
        $(this).click(function() {
            octaveShift = parseInt($(this).attr('octaveShift'));
            socket.emit('ui', {
                address: 'instrument',
                args: [
                    {
                        type: 'integer',
                        value: parseInt($(this).attr('bank'))
                    },
                    {
                        type: 'integer',
                        value: parseInt($(this).attr('instrument'))
                    }
                ]
            });
        });
    });

    $('[solochannel]').each(function() {

        var soloBtnId = makeId();
        $(this).attr('id', soloBtnId);

        var soloButton = new Nexus.Button('#' + soloBtnId, {
            'size': [30, 30],
            'mode': 'toggle',  // 'button', 'toggle', or 'impulse'
            'state': false
        });

        soloButton.on('change',function(v) {
            socket.emit('ui', {
                address: 'solochannel',
                args: [
                    {
                        type: 'integer',
                        value: parseInt($('#' + soloBtnId).attr('solochannel'))
                    },
                    {
                        type: 'integer',
                        value: v ? 1 : 0
                    }
                ]
            });
        });
    });

    // Get widgets
    updateAllWidgets();

    $('.grid').masonry({
        // fitWidth: true
        horizontalOrder: false,
        columnWidth: 1 // ???
    });
});

// Prevent unwanted refresh on mobile
document.body.addEventListener('touchmove', function(event) {
    event.preventDefault();
}, false);

function updateAllWidgets() {
    socket.emit('ui', {
        address: 'getwidgets',
        args: []
    });
}