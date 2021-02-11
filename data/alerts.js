// Common file for AJAX errors

settingsAlert = function () { }
settingsAlert.warning = function (message) {
    _div = "";
    if (message) {
        _div = '<div class="alert alert-dismissible alert-danger" id="warnSettingsError">'
        _div += '<button type="button" class="close" data-dismiss="alert">&times;</button><p class="mb-0">'
        _div += '<strong>Settings Error: </strong><span>' + message + '</span></p></div>'
    }
    $('#settingsAlert_placeholder').html(_div);
}
settingsAlert.error = function (message) {
    _div = "";
    if (message) {
        _div = '<div class="alert alert-dismissible alert-danger" id="warnSettingsError">'
        _div += '<button type="button" class="close" data-dismiss="alert">&times;</button><p class="mb-0">'
        _div += '<strong>Settings Error: </strong><span>' + message + '</span></p></div>'
    }
    $('#settingsAlert_placeholder').html(_div);
}
