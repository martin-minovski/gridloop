
function getCurrentScale() {
    // var body = document.querySelector('.zoomContainer');
    // return body.getBoundingClientRect().width / body.offsetWidth;
    return 1;
}

function makeId() {
    var text = "";
    var possible = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

    for (var i = 0; i < 5; i++)
        text += possible.charAt(Math.floor(Math.random() * possible.length));

    return text;
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