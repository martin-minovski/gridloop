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
        coverflowEffect: {
            rotate: 30,
            slideShadows: false,
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

    editor.setFontSize(20);

    $(window).keypress(function (e) {
        if (!isEditorOpen) {
            if (e.key === ' ' || e.key === 'Spacebar') {

                e.preventDefault();
            }
        }
    })

    // Setup complete
});