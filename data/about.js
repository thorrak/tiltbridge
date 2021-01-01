//
// Created by Lee Bussy on 12/31/20
//

var unloadingState = false;
var numReq = 3;
var loaded = 0;
var heapReloadTimer = 60000;

// Attach the event after the page loads
if (window.addEventListener)
    window.addEventListener("load", startLoad, false);
else if (window.attachEvent)
    window.attachEvent("onload", startLoad);
else window.onload = startLoad;

function startLoad() { // Make sure the page is 100% loaded
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

// Detect unloading state during getJSON
$(window).bind("beforeunload", function () {
    unloadingState = true;
});

function populatePage() { // Get page data
    $(document).tooltip({ // Enable tooltips
        'selector': '[data-toggle=tooltip]',
        'toggleEnabled': true
    });

    heapToolTip();      // Set up tooltip for debug info

    loadThisVersion();  // Populate form with controller settings

    loadUptime();       // Load uptime information
    loadHeap();         // Load heap information
    loadResetReason();  // Load last reset reason

    pollComplete();
}

function heapToolTip() {
    var heapToolTip = "Heap Information:<br>";
    heapToolTip += "<ul>";
    heapToolTip += "<li>Free Heap = Total free bytes in the heap";
    heapToolTip += "<li>Max = Size of largest free block in the heap";
    heapToolTip += "<li>Frags = 100 - (max * 100) / free";
    heapToolTip += "</ul>";
    $("#uptime").attr("data-original-title", "Time since last controller (re)start");
    $("#resetreason").attr("data-original-title", "Reason for last (re)start");
    $("#heap").attr("data-original-title", heapToolTip);
}

function loadThisVersion() { // Get current parameters
    var thisVersionJson = "/thisVersion/";
    var thisVersion = $.getJSON(thisVersionJson, function () {
    })
        .done(function (thisVersion) {
            try {
                $('#thisVersion').text("v" + thisVersion.version);
                $('#thisBranch').text(thisVersion.branch);
                $('#thisBuild').text(thisVersion.build);
            }
            catch {
                $('#thisVersion').html("").html('<span class="text-danger">Error parsing version.</span>');
                $('#thisBranch').text();
                $('#thisBuild').text();
            }
        })
        .fail(function () {
            $('#thisVersion').html("").html('<span class="text-danger">Error loading version.</span>');
            $('#thisBranch').text();
            $('#thisBuild').text();
        })
        .always(function () {
            // Can post-process here
        });
}

function loadUptime(callback = null) { // Get uptime information
    var uptimeJson = "/uptime/";
    var uptime = $.getJSON(uptimeJson, function () {
    })
        .done(function (uptime) {
            try {
                var days = uptime.u.days.toString();
                var hours = uptime.u.hours.toString();
                var minutes = uptime.u.minutes.toString();
                var seconds = uptime.u.seconds.toString();

                var uptime = "Days: " + days + ", Hours: " + hours + ", Minutes: " + minutes + ", Seconds: " + seconds;
                $('#uptime').text(uptime);
            }
            catch {
                $('#uptime').text("(Error parsing uptime.)");
            }
        })
        .fail(function () {
            $('#uptime').text("(Error loading uptime.)");
        })
        .always(function () {
            if (loaded < numReq) {
                loaded++;
            }
            if (typeof callback == "function") {
                callback();
            }
        });
}

function loadHeap(callback = null) { // Get heap information
    var heapJson = "/heap/";
    var heap = $.getJSON(heapJson, function () {
    })
        .done(function (heap) {
            try {
                var free = heap.h.free;
                var max = heap.h.max;
                var frag = heap.h.frag;

                var heapinfo = "Free Heap: " + free + ", Max: " + max + ", Frags: " + frag;
                $('#heap').text(heapinfo);
            }
            catch {
                $('#heap').text("(Error parsing heap.)");
            }
        })
        .fail(function () {
            $('#heap').text("(Error loading heap.)");
        })
        .always(function () {
            if (loaded < numReq) {
                loaded++;
            }
            if (typeof callback == "function") {
                callback();
            }
        });
}

function loadResetReason(callback = null) { // Get last reset reason
    var resetJson = "/resetreason/";
    var reset = $.getJSON(resetJson, function () {
    })
        .done(function (reset) {
            try {
                var resetReason = reset.r.reason;
                var resetDescription = reset.r.description;

                var resetText = "Reason: " + resetReason + ", Description: " + resetDescription;
                $('#resetreason').text(resetText);
            }
            catch {
                $('#resetreason').text("(Error parsing version.)");
            }
        })
        .fail(function () {
            $('#resetreason').text("(Error loading version.)");
        })
        .always(function () {
            if (loaded < numReq) {
                loaded++;
            }
            if (typeof callback == "function") {
                callback();
            }
        });
}

function pollComplete() {
    if (loaded == numReq) {
        finishPage();
    } else {
        setTimeout(pollComplete, 300); // try again in 300 milliseconds
    }
}

function heapReload() {
    loadHeap(function callFunction() {
        setTimeout(heapReload, heapReloadTimer);
    });
}

function uptimeReload() {
    loadUptime(function callFunction() {
        setTimeout(uptimeReload, heapReloadTimer);
    });
}

function reasonReload() {
    loadResetReason(function callFunction() {
        setTimeout(reasonReload, heapReloadTimer);
    });
}

function finishPage() { // Display page
    setTimeout(heapReload, heapReloadTimer);
    setTimeout(uptimeReload, heapReloadTimer);
    setTimeout(reasonReload, heapReloadTimer);
}
