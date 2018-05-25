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
                else if (
                    axisX.type === 'sync' ||
                    axisX.type === 'theremin' ||
                    axisX.type.indexOf('fsr') > -1 ||
                    axisX.type.indexOf('imu') > -1) {
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