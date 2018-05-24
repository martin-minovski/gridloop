var editor = ace.edit("faust-editor");
editor.setFontSize(20);
var editingChannel = 0;
var editRequested = false;
var volumeSliders = [];
var soloButtons = [];
var preventSend = false;
var midiRec = false;
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

var channelColors = [
    '#00aedb',
    '#a200ff',
    '#f47835',
    '#d41243',
    '#8ec127',
    '#ff7373',
    '#caaac0',
    '#edd97c'

]

var LOOPER_CHANNELS = 8;
var looperChannels = LOOPER_CHANNELS;
var faustWidgets = [];
var widgetIDs = [];
var instruments = {};

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
var saveRequested = false;
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
}

function faustClose() {
    $('#faust-editor-container').hide();
    setBlur(0);
}

var socket = io('/');
socket.on('cppinput', function (data) {
    if (data.address === 'json_channelsummary') {
        for (var i = 0; i < 8; i++) {
            for (var j = 0; j < 4; j++) {
                gridItems[i][j].element.find('.square-variation').hide();
            }
        }
        var channels = JSON.parse(data.args[0].value);
        if (channels) channels.forEach(function(channel, index) {
            if (channel.variation !== 0) {
                gridItems[index][channel.variation].element.find('.square-variation').show();
            }

            volumeSliders[index].value = channel.volume;
            soloButtons[index].state = channel.solo;
        });
    }
    if (data.address === 'active') {
        var channel = data.args[0].value;
        var variation = data.args[1].value;
        gridItems[activeChannel][activeVariation].element.css('border-radius', '1px').find('.square-cover').css('border-radius', '1px');
        activeChannel = channel;
        activeVariation = variation;
        gridItems[channel][variation].element.css('border-radius', '100px').find('.square-cover').css('border-radius', '100px');
    }
    if (data.address === 'json_clipsummary') {
        for (var i = 0; i < 8; i++) {
            for (var j = 0; j < 4; j++) {
                gridItems[i][j].element.find('.square-cover').show();
                gridItems[i][j].element.find('.square-master').hide();
                gridItems[i][j].element.find('.square-counter').html('');
                gridItems[i][j].clipCount = 0;
            }
        }
        var clips = JSON.parse(data.args[0].value);
        if (clips) clips.forEach(function(clip) {
            var channel = clip.channel;
            var variation = clip.variation;
            var length = clip.length;
            var master = clip.master;
            gridItems[channel][variation].clipCount++;
            gridItems[channel][variation].element.find('.square-counter').html(gridItems[channel][variation].clipCount);
            gridItems[channel][variation].element.find('.square-cover').hide();
            if (master) gridItems[channel][variation].element.find('.square-master').show();
        });
    }
    if (data.address === 'faust_ack') {
        if (!saveRequested) return;
        saveRequested = false;
        updateAllWidgets();
        alert("Compiled with no errors!");
    }
    if (data.address === 'faust_code') {
        if (!editRequested) return;
        editRequested = false;
        var channel = data.args[0].value;
        var code = data.args[1].value;
        faustOpen(channel, code);
    }
    if (data.address === 'faust_error') {
        if (!saveRequested) return;
        saveRequested = false;
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

        // console.log("Widgets array: ", faustWidgets);

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
                if (!axisX) axisX = axisY;
                if (!axisX) axisX = axisZ;
                if (!axisX) continue;

                var widgetDiv = $('<div style="font-size: 10px;">' + j + '<br /><div class="widget-div" id="' + widgetID + '"></div></div>');
                widgetContainer.append(widgetDiv);

                if (axisX.type === 'slider') {
                    var nexusUiWidget = new Nexus.Slider('#' + widgetID, {
                        'size': [130,30],
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
                    styllize(nexusUiWidget);
                }
                else if (axisX.type === 'xypad') {
                    var nexusUiWidget = new Nexus.Position('#' + widgetID, {
                        'size': [130,130],
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
                    styllize(nexusUiWidget);
                    $('#' + widgetID).addClass('rounded-widget-container');
                    $('#' + widgetID).addClass('xypad');
                }
                else if (axisX.type === 'tilt') {
                    var nexusUiWidget = new Nexus.Tilt('#' + widgetID);
                    nexusUiWidget.active = false;

                    nexusUiWidget.on('change',function(value) {
                        var widget = widgetIDs[this.settings.target];
                        if (widget[0] && value.x)
                            updateFaustZone(widget[0].zone, value.x * (widget[0].max - widget[0].min) + widget[0].min);
                        if (widget[1] && value.y)
                            updateFaustZone(widget[1].zone, value.y * (widget[1].max - widget[1].min) + widget[1].min);
                        if (widget[2] && value.z)
                            updateFaustZone(widget[2].zone, value.z * (widget[2].max - widget[2].min) + widget[2].min);
                    });
                    styllize(nexusUiWidget);
                    $('#' + widgetID).css('margin-left', '25px').addClass('rounded-widget-container');
                }
                else if (axisX.type === 'button') {
                    var nexusUiWidget = new Nexus.Button('#' + widgetID, {
                        'size': [50,50],
                        'mode': 'aftertouch',
                        'state': false
                    });
                    nexusUiWidget.on('change',function(value) {
                        var widget = widgetIDs[this.settings.target];
                        if (widget[0])
                            updateFaustZone(widget[0].zone, value.state ? widget[0].max : widget[0].min);
                        if (widget[1])
                            updateFaustZone(widget[1].zone, value.x * (widget[1].max - widget[1].min) + widget[1].min);
                        if (widget[2])
                            updateFaustZone(widget[2].zone, value.y * (widget[2].max - widget[2].min) + widget[2].min);
                    });
                    styllize(nexusUiWidget);
                    $('#' + widgetID).css('margin-left', '45px');
                }
                else if (axisX.type === 'toggle') {
                    var nexusUiWidget = new Nexus.Toggle('#' + widgetID, {
                        'size': [60, 30],
                        'state': false
                    });
                    nexusUiWidget.on('change',function(value) {
                        var widget = widgetIDs[this.settings.target];
                        if (widget[0])
                            updateFaustZone(widget[0].zone, value ? widget[0].max : widget[0].min);
                    });
                    styllize(nexusUiWidget);
                    $('#' + widgetID).css('margin-left', '40px');
                }
                else if (axisX.type === 'sync' || axisX.type.indexOf('fsr') > -1) {
                    $('#' + widgetID).html(axisX.type.toUpperCase() + "<br /><br />")
                        .css('font-weight', '600')
                        .css('font-size', '14px')
                        .css('margin-bottom', '-8px');
                }

            }
        }
    }
    if (data.address === 'json_instruments') {
        var jsonString = data.args[0].value;
        var data = JSON.parse(jsonString);
        instruments = data;
        var dropdownStrings = [];
        $.each(data, function(i, bank) {
            $.each(bank, function(j, instrument) {
                dropdownStrings.push(i + '/' + j + " " + instrument);
            });
        });
        $('#instrument-select').html('');
        var instrumentSelect = new Nexus.Select('#instrument-select',{
            'size': [120,25],
            'options': dropdownStrings
        })
        styllize(instrumentSelect);
        instrumentSelect.on('change',function(value) {
            console.log(value.value);
            var data = value.value.split(' ')[0];
            console.log(data);
            data = data.split('/');
            var bankNum = data[0];
            var instrNum = data[1];
            socket.emit('ui', {
                address: 'instrument',
                args: [
                    {
                        type: 'integer',
                        value: parseInt(bankNum)
                    },
                    {
                        type: 'integer',
                        value: parseInt(instrNum)
                    }
                ]
            });
        });

    }

});

var octaveShift = 0;

$(document).ready(function() {
    initWidgetsArray();
    var viewport = $('#mainContainer');
    for (var i = 0; i < looperChannels; i++) {
        var channelElement = $('<div style="background-color: ' + channelColors[i] + '" class="grid-item-channel item grid-item-padded">' +
            '<div class="title"><div style="font-size: 10px; font-weight: 600">Channel ' + (i+1) + ' ' +
            '<i class="fa fa-pencil-alt" onclick="editDSP(' + i + ')" style="cursor: pointer"></i>' +
            '</div></div>' +
            '<div id="widget-container-' + i + '"></div>' +
            '</div>');
        viewport.append(channelElement);
    }
    viewport.masonry();

    var piano = new Nexus.Piano('#piano', {
        'size': [340, 140],
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
    styllize(piano);

    var recButton = new Nexus.Button('#looper-rec',{
        'size': [40, 40],
        'mode': 'toggle',  // 'button', 'toggle', or 'impulse'
        'state': false
    });

    recButton.on('change',function(v) {
        socket.emit('ui', {
            address: 'rec',
            args: [
                {
                    type: 'integer',
                    value: v ? 1 : 0
                },
                {
                    type: 'integer',
                    value: midiRec ? 0 : 1
                }
            ]
        });
    });
    styllize(recButton);

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
        channelArea.append($('<div style="height:5px;position:relative;"><div style="position:absolute;margin-top:11px;margin-left:9px;pointer-events:none;color:#bbb">S</div></div>'));
        channelArea.append($('<div solochannel="' + i + '"></div>'));
        $('#looper-channel-holder').append(channelArea);
    }
    // var chSelect = new Nexus.Select('#looper-ch',{
    //     'size': [100,30],
    //     'options': chArray
    // });
    // chSelect.on('change',function(data) {
    //     var index = data.index;
    //     var value = data.value;
    //     socket.emit('ui', {
    //         address: 'looperchannel',
    //         args: [
    //             {
    //                 type: 'integer',
    //                 value: index
    //             }
    //         ]
    //     });
    // });

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
        styllize(gainSlider);
        volumeSliders[meta] = gainSlider;
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
        var meta = $(this).attr('solochannel');
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
        styllize(soloButton);
        soloButtons[meta] = soloButton;
    });

    // Get widgets
    updateAllWidgets();

    // Get instruments
    getInstruments();

    $('.grid').masonry({
        fitWidth: true,
        horizontalOrder: false,
        columnWidth: 1 // ???
    });

    var vocoderSwitch = new Nexus.Toggle('#vocoder-switch', {
        'size': [20, 20],
        'state': false
    });
    vocoderSwitch.on('change',function(value) {
        socket.emit('ui', {
            address: 'vocoderswitch',
            args: [
                {
                    type: 'integer',
                    value: value ? 1 : 0
                }
            ]
        });
    });
    styllize(vocoderSwitch);

    var wrapper = $('.loop-grid-wrapper');
    for (var i = 0; i < 8*4; i++) {
        var thisChannel = i % 8;
        var thisVariation = Math.floor(i / 8);
        // if (i % 8 === 0 && i !== 0) {
        //     wrapper.append($('<br />'));
        // }
        var squareChar = thisVariation === 0 ? 'X' : String.fromCharCode(64 + thisVariation);
        var square = $(`
        <div class="square" onpointerdown="activateChannel(`+ thisChannel + ',' + thisVariation + `);">
            <div class="square-label">` + squareChar + `</div>
            <div class="square-cover"></div>
            <div class="square-counter"></div>
            <div class="square-master">M</div>
            <div class="square-variation"></div>
        </div>
        `);
        if (thisVariation === 0)  square.append($('<div class="square-variation-constant"></div>'));
        square.css('background-color', channelColors[thisChannel]);
        gridItems[thisChannel][thisVariation].element = square;
        wrapper.append(square);
    }

    // Initialize Swiper
    var swiper = new Swiper('.swiper-container', {
        pagination: {
            el: '.swiper-pagination'
        },
        simulateTouch: false,
        navigation: {
            nextEl: '.swiper-button-next',
            prevEl: '.swiper-button-prev'
        }
    });

    var midiRecSwitch = new Nexus.Toggle('#midi-rec-switch', {
        'size': [20, 20],
        'state': false
    });
    midiRecSwitch.on('change',function(value) {
        midiRec = value;
    });
    styllize(midiRecSwitch);

    getClipSummary();
    getChannelSummary();
    getActive();

    // Document Load Tail
});

var gridItems = [];

for (var i = 0; i < 8; i++) {
    gridItems[i] = [];
    for (var j = 0; j < 4; j++) {
        gridItems[i][j] = {
            element: undefined
        };
    }
}

var activeChannel = 0;
var activeVariation = 0;
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

// Prevent unwanted refresh on mobile
// document.body.addEventListener('touchmove', function(event) {
//     event.preventDefault();
// }, false);

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

function goFullscreen() {
    var mainBody = document.documentElement;
    var i = mainBody;
    if (i.requestFullscreen) {
        i.requestFullscreen();
    } else if (i.webkitRequestFullscreen) {
        i.webkitRequestFullscreen();
    } else if (i.mozRequestFullScreen) {
        i.mozRequestFullScreen();
    } else if (i.msRequestFullscreen) {
        i.msRequestFullscreen();
    }
    $('.zoomContainer').trigger( "click" );
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