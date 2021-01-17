// Supports settings page

jQuery('#overlay').fadeIn();
var unloadingState = false;
var loaded = 0; // Hold data load status
var numReq = 1; // Number of JSON queries required
var hostname = window.location.hostname;
var newHostName;
var originalHostnameConfig;
var newtarget;
var pingtarget;
var hashLoc;
var didreset = false;
var posted = false;

// Tab tracking
var previousTab = "";
var currentTab = "";

// Attach the event after the page loads (multi-browser)
if (window.addEventListener)
    window.addEventListener("load", loadPage, false);
else if (window.attachEvent)
    window.attachEvent("onload", loadPage);
else window.onload = loadPage;

function loadPage() { // Page data load - make triple sure the whole DOM loaded
    if (document.readyState === 'ready' || document.readyState === 'complete') {
        populatePage();
    } else {
        document.onreadystatechange = function () {
            if (document.readyState == "complete") {
                populatePage();
            }
        }
    }
}

$(window).bind("beforeunload", function () { // Handle unloading page while making a getJSON call
    unloadingState = true;
});

$('a[data-toggle="tab"]').on('shown.bs.tab', function (event) { // Actions as tabs change
    previousTab = currentTab;
    currentTab = $(event.target).text();

    // Handle form changes
    // if (previousTab == "Xxxx") {
    // Do something
    // } else if (currentTab == "Xxx") {
    // Do something
    // }
    var url = $(event.target).attr("href") // URL of activated tab
    var hashLoc = url.substring(url.indexOf('#')); // Get hash
    updateHelp(hashLoc); // Set context-sensitive help
});

$('input[type=radio]').change(function () { // Turn off tooltips on radio button change
    $('[data-toggle="tooltip"], .tooltip').tooltip("hide");
});
$('input[type=checkbox]').change(function () { // Turn off tooltips on checkbox change
    $('[data-toggle="tooltip"], .tooltip').tooltip("hide");
});

function populatePage() { // Prepopulate form data
    $(document).tooltip({
        'selector': '[data-toggle=tooltip]',
        'toggleEnabled': true
    });
    populateConfig();
    loadHash();
    pollComplete();
}

function repopulatePage(doSpinner = false) { // Reload data on demand
    // doSpinner = true turns on loading graphic while the load happens
    if (doSpinner) {
        jQuery('#overlay').fadeIn();
    }
    loaded = 0;
    populateConfig();
    pollComplete();
}

function loadHash() { // Link to tab via hash value
    var url = document.location.toString();
    if (url.match('#')) {
        $('.nav-tabs a[href="#' + url.split('#')[1] + '"]').tab('show');
    }

    // Change hash for page-reload
    $('.nav-tabs a').on('shown.bs.tab', function (e) {
        window.location.hash = e.target.hash;
    });
}

function populateConfig(callback = null) { // Get configuration settings, populate forms
    var url = "/settings/json/";
    var config = $.getJSON(url, function () {
        settingsAlert.warning()
    })
        .done(function (config) {
            try {
                // Clear errors
                settingsAlert.warning();
                settingsAlert.error();
                // Store hostname before er (possibly) change it
                originalHostnameConfig = config.mdnsID;

                // TiltBridge Tab
                $('input[name="mdnsID"]').val(config.mdnsID);
                // offset = new Date().getTimezoneOffset(); // TODO: Can just pull TZOffset from client
                // offset = (offset / 60) *= -1; // Offset is in minutes and "inverted"
                $('input[name="TZoffset"]').val(config.TZoffset);
                $('select[name="tempUnit"] option[value=' + config.tempUnit + ']').attr('selected', 'selected');
                $('input[name="smoothFactor"]').val(config.smoothFactor);
                if (config.invertTFT) {
                    $('input[name="invertTFT"]').prop("checked", true);
                } else {
                    $('input[name="invertTFT"]').prop("checked", false);
                }

                // Calibration Tab

                if (config.applyCalibration) {
                    $('input[name="applyCalibration"]').prop("checked", true);
                } else {
                    $('input[name="applyCalibration"]').prop("checked", false);
                }
                if (config.tempCorrect) {
                    $('input[name="tempCorrect"]').prop("checked", true);
                } else {
                    $('input[name="tempCorrect"]').prop("checked", false);
                }

                // Local Target Tab
                $('input[name="localTargetURL"]').val(config.localTargetURL);
                $('input[name="localTargetPushEvery"]').val(config.localTargetPushEvery);

                // Google Sheets Tab
                $('input[name="scriptsURL"]').val(config.scriptsURL);
                $('input[name="scriptsEmail"]').val(config.scriptsEmail);
                $('input[name="sheetName_red"]').val(config.sheetName_red);
                $('input[name="sheetName_green"]').val(config.sheetName_green);
                $('input[name="sheetName_black"]').val(config.sheetName_black);
                $('input[name="sheetName_purple"]').val(config.sheetName_purple);
                $('input[name="sheetName_orange"]').val(config.sheetName_orange);
                $('input[name="sheetName_blue"]').val(config.sheetName_blue);
                $('input[name="sheetName_yellow"]').val(config.sheetName_yellow);
                $('input[name="sheetName_pink"]').val(config.sheetName_pink);

                // Brewer's Friend Tab
                $('input[name="brewersFriendKey"]').val(config.brewersFriendKey);

                // Brewfather Tab
                $('input[name="brewfatherKey"]').val(config.brewfatherKey);

                // Brewstatus Tab
                $('input[name="brewstatusURL"]').val(config.brewstatusURL);
                $('input[name="brewstatusPushEvery"]').val(config.brewstatusPushEvery);

                // MQTT Tab
                $('input[name="mqttBrokerHost"]').val(config.mqttBrokerHost);
                $('input[name="mqttBrokerPort"]').val(config.mqttBrokerPort);
                $('input[name="mqttUsername"]').val(config.mqttUsername);
                $('input[name="mqttPassword"]').val(config.mqttPassword);
                $('input[name="mqttTopic"]').val(config.mqttTopic);
                $('input[name="mqttPushEvery"]').val(config.mqttPushEvery);
            }
            catch {
                if (!unloadingState) {
                    settingsAlert.warning("Unable to parse configuration data.");
                }
            }
        })
        .fail(function () {
            if (!unloadingState) {
                settingsAlert.error("Unable to retrieve configuration data.");
            }
        })
        .always(function () {
            // Can post-process here
            if (loaded < numReq) {
                loaded++;
            }
            if (typeof callback == "function") {
                callback();
            }
        });
}

function isIP(hostname) { // Bool: is this an IP address
    if (/^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/.test(hostname)) {
        return (true)
    } else {
        return (false)
    }
}

function isMDNS(hostname) { // Bool: Is this an mDNS host name
    if (hostname.endsWith(".local")) {
        return (true);
    } else {
        return (false);
    }
}

function pollComplete() { // Poll to see if entire page is loaded
    if (loaded == numReq) {
        posted = true;
        finishPage();
    } else {
        setTimeout(pollComplete, 300); // try again in 300 milliseconds
    }
}

function finishPage() { // Display page
    jQuery('#overlay').fadeOut();
}

// POST Handlers:

document.querySelectorAll('.button-update').forEach(item => {
    item.addEventListener('click', event => {
        event.preventDefault(); // Prevent form submission
        processPost(event.target); // Send the element and process the POST
    })
});

function processPost(obj) { // Disable buttons and call POST handler for form
    posted = false;
    hashLoc = window.location.hash;
    var $form = $(obj.form);
    url = $form.attr("action");

    // Disable buttons, show a spinner
    $(".button-update").prop('disabled', true);
    $(".button-update").html('<i class="fa fa-spinner fa-spin"></i> Updating');
    $(".button-resetwifi").prop('disabled', true);
    $(".button-resetwifi").html('<i class="fa fa-spinner fa-spin"></i> Updating')
    $(".button-resetcontroller").prop('disabled', true);
    $(".button-resetcontroller").html('<i class="fa fa-spinner fa-spin"></i> Updating')
    $(".button-resetfactory").prop('disabled', true);
    $(".button-resetfactory").html('<i class="fa fa-spinner fa-spin"></i> Updating')

    switch (hashLoc) { // Switch here for the tab we are processing
        case "#tiltbridge":
            processControllerPost(url, obj);
            break;
        case "#calibration":
            processCalibrationPost(url, obj);
            break;
        case "#localtarget":
            processLocalTargetPost(url, obj);
            break;
        case "#googlesheets":
            processGoogleSheetsPost(url, obj);
            break;
        case "#brewersfriend":
            processBrewersFriendPost(url, obj);
            break;
        case "#brewfather":
            processBrewfatherPost(url, obj);
            break;
        case "#brewstatus":
            processBrewstatusPost(url, obj);
            break;
        case "#mqtt":
            processMqttPost(url, obj);
            break;
        default:
            // Unknown hash location passed
            break;
    }
    buttonClearDelay();
}

function processControllerPost(url, obj) { // Process a post from the TiltBridge tab
    // Get form data
    var $form = $(obj.form),
        hostnameVal = $form.find("input[name='mdnsID']").val(),
        tzOffsetVal = $form.find("input[name='TZoffset']").val(),
        tempUnitVal = $form.find("select[name='tempUnit']").val(),
        smoothFactorVal = $form.find("input[name='smoothFactor']").val(),
        invertTFTVal = $form.find('input[name="invertTFT"]').is(":checked");

    // Hold some data about what we changed
    var reloadpage = false;
    var hostnamechanged = false;

    // Decide if we accessed via IP or non-mDNS name
    var confirmText = '';
    if (hostnameVal != originalHostnameConfig) {
        hostnamechanged = true;
        if (isIP(hostname)) {
            confirmText = 'You have connected with an IP address. Changing the hostname may have unintended consequences. Do you want to proceed?';
            reloadpage = false;
        } else if (!isMDNS(hostname)) {
            confirmText = 'You are not using an mDNS name. Changing the hostname may have unintended consequences. Do you want to proceed?';
            reloadpage = false;
        } else {
            reloadpage = true;
        }
    }
    if (confirmText && (!confirm(confirmText))) {
        // Bail out on post
        return;
    } else {
        // Process post
        originalHostnameConfig = hostnameVal; // Pick up changed host name
        data = {
            mdnsID: hostnameVal,
            tzOffset: tzOffsetVal,
            tempUnit: tempUnitVal,
            smoothFactor: smoothFactorVal,
            invertTFT: invertTFTVal,
        }
        if (hostnamechanged && reloadpage) {
            jQuery('#overlay').fadeIn();
            var protocol = window.location.protocol;
            var path = window.location.pathname;
            newtarget = protocol + "//" + hostnameVal + ".local" + path + hashLoc;
            pingtarget = protocol + "//" + hostnameVal + ".local/favicon.ico";
            postData(url, data, true);
        } else {
            postData(url, data, false, false, function () {
                jQuery('#overlay').fadeOut();
            });
        }
    }
}

function processCalibrationPost(url, obj) { // Handle Calibration Tab posts
    // Get form data
    var $form = $(obj.form),
        applyCalibrationVal = $form.find('input[name="applyCalibration"]').is(":checked"),
        tempCorrectVal = $form.find('input[name="tempCorrect"]').is(":checked")

    // Process post
    data = {
        applyCalibration: applyCalibrationVal,
        tempCorrect: tempCorrectVal
    };
    postData(url, data);
}

function processLocalTargetPost(url, obj) { // Handle Target URL posts
    // Get form data
    var $form = $(obj.form),
        localTargetURLVal = $form.find("input[name='localTargetURL']").val(),
        localTargetPushEveryVal = $form.find("input[name='localTargetPushEvery']").val();

    // Process post
    data = {
        localTargetURL: localTargetURLVal,
        localTargetPushEvery: localTargetPushEveryVal
    };
    postData(url, data);
}

function processGoogleSheetsPost(url, obj) { // Handle Google Sheets posts
    // Get form data
    var $form = $(obj.form),
        scriptsURLVal = $form.find("input[name='scriptsURL']").val(),
        scriptsEmailVal = $form.find("input[name='scriptsEmail']").val(),
        sheetName_redVal = $form.find("input[name='sheetName_red']").val(),
        sheetName_greenVal = $form.find("input[name='sheetName_green']").val(),
        sheetName_blackVal = $form.find("input[name='sheetName_black']").val(),
        sheetName_purpleVal = $form.find("input[name='sheetName_purple']").val(),
        sheetName_orangeVal = $form.find("input[name='sheetName_orange']").val(),
        sheetName_blueVal = $form.find("input[name='sheetName_blue']").val(),
        sheetName_yellowVal = $form.find("input[name='sheetName_yellow']").val(),
        sheetName_pinkVal = $form.find("input[name='sheetName_pink']").val();

    // Process post
    data = {
        scriptsURL: scriptsURLVal,
        scriptsEmail: scriptsEmailVal,
        sheetName_red: sheetName_redVal,
        sheetName_green: sheetName_greenVal,
        sheetName_black: sheetName_blackVal,
        sheetName_purple: sheetName_purpleVal,
        sheetName_orange: sheetName_orangeVal,
        sheetName_blue: sheetName_blueVal,
        sheetName_yellow: sheetName_yellowVal,
        sheetName_pink: sheetName_pinkVal
    };
    postData(url, data);
}

function processBrewersFriendPost(url, obj) { // Handle Brewer's Friend posts


    // Get form data
    var $form = $(obj.form),
        brewersFriendKeyVal = $form.find("input[name='brewersFriendKey']").val();

    // Process post
    data = {
        brewersFriendKey: brewersFriendKeyVal
    };
    postData(url, data);
}

function processBrewfatherPost(url, obj) { // Handle Brewfather posts
    // Get form data
    var $form = $(obj.form),
        brewfatherKeyVal = $form.find("input[name='brewfatherKey']").val();

    // Process post
    data = {
        brewfatherKey: brewfatherKeyVal
    };
    postData(url, data);
}

function processBrewstatusPost(url, obj) { // Handle Brewstatus posts
    // Get form data
    var $form = $(obj.form),
        brewstatusURLVal = $form.find("input[name='brewstatusURL']").val(),
        brewstatusPushEveryVal = $form.find("input[name='brewstatusPushEvery']").val();

    // Process post
    data = {
        brewstatusURL: brewstatusURLVal,
        brewstatusPushEvery: brewstatusPushEveryVal
    };
    postData(url, data);
}

function processMqttPost(url, obj) { // Handle MQTT posts
    // Get form data
    var $form = $(obj.form),
        mqttBrokerHostVal = $form.find("input[name='mqttBrokerHost']").val(),
        mqttBrokerPortVal = $form.find("input[name='mqttBrokerPort']").val(),
        mqttUsernameVal = $form.find("input[name='mqttUsername']").val(),
        mqttPasswordVal = $form.find("input[name='mqttPassword']").val(),
        mqttTopicVal = $form.find("input[name='mqttTopic']").val(),
        mqttPushEveryVal = $form.find("input[name='mqttPushEvery']").val();

    // Process post
    data = {
        mqttBrokerHost: mqttBrokerHostVal,
        mqttBrokerPort: mqttBrokerPortVal,
        mqttUsername: mqttUsernameVal,
        mqttPassword: mqttPasswordVal,
        mqttTopic: mqttTopicVal,
        mqttPushEvery: mqttPushEveryVal
    };
    postData(url, data);
}

function postData(url, data, newpage = false, newdata = false, callback = null) { // POST data; newpage = reload, newdata = pull new data
    $.ajax({
        url: url,
        type: 'POST',
        data: data,
        success: function (data) {
            settingsAlert.warning();
        },
        error: function (data) {
            settingsAlert.warning("Settings update failed, check your entries.");
        },
        complete: function (data) {
            if (newpage) {
                waitOnReset();
            } else if (newdata) {
                repopulatePage(true);
            }
            posted = true;
            if (typeof callback == "function") {
                callback();
            }
        }
    });
}

function buttonClearDelay() { // Poll to see if entire page is loaded
    if (posted) {
        $(".button-update").prop('disabled', false);
        $(".button-update").html('Update');
        $(".button-resetcontroller").prop('disabled', true);
        $(".button-resetcontroller").html('Restart Controller');
        $(".button-resetwifi").prop('disabled', true);
        $(".button-resetwifi").html('Reset Wifi');
        $(".button-resetfactory").prop('disabled', true);
        $(".button-resetfactory").html('Factory Reset');
        posted = false;
    } else {
        setTimeout(buttonClearDelay, 500); // try again in 500 milliseconds
    }
}

function updateHelp(hashLoc) {
    // TODO:  Can set context sensitive help for page
    var url = "https://docs.tiltbridge.com"

    // Switch here for hashLoc
    switch (hashLoc) {
        case "#tiltbridge":
            url = url + "";
            break;
        case "#calibration":
            url = url + "";
            break;
        case "#localtarget":
            url = url + "";
            break;
        case "#googlesheets":
            url = url + "";
            break;
        case "#brewersfriend":
            url = url + "";
            break;
        case "#brewfather":
            url = url + "";
            break;
        case "#brewstatus":
            url = url + "";
            break;
        case "#mqtt":
            url = url + "";
            break;
        default:
            // Unknown hash location passed
            break;
    }
    $("#contexthelp").prop("href", url)
}

function waitOnReset() {
    // Wait for restart to complete
    window.setInterval(function () {
        checkServerStatus(function (semaphore) {
            didreset = semaphore;
            if (didreset == true) {
                // Reset is complete
                setTimeout(function () {
                    window.location.href = newtarget;
                }, 1000);
            }
        });
    }, 1000);
}

function checkServerStatus(callback) {
    var img = document.body.appendChild(document.createElement("img"));
    img.onload = function () {
        callback(true);
    };
    img.onerror = function () {
        callback({});
    };
    img.src = pingtarget;
}
