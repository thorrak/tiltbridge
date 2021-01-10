//
// Created by Lee Bussy on 12/31/20
//

var unloadingState = false;
var numReq = 1;
var loaded = 0;
var aboutReloadTimer = 60000;

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

    loadAboutInfo();    // Load uptime information

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

function loadAboutInfo(callback = null) { // Get uptime information
    var aboutJson = "/aboutinfo/";
    var about = $.getJSON(aboutJson, function () {
    })
        .done(function (about) {
            try {
                $('#thisVersion').text("v" + about.version);
                $('#thisBranch').text(about.branch);
                $('#thisBuild').text(about.build);

                var days = about.days.toString();
                var hours = about.hours.toString();
                var minutes = about.minutes.toString();
                var seconds = about.seconds.toString();

                var uptime = "Days: " + days + ", Hours: " + hours + ", Minutes: " + minutes + ", Seconds: " + seconds;
                $('#uptime').text(uptime);

                var free = about.free;
                var max = about.max;
                var frag = about.frag;
                var heapinfo = "Free Heap: " + free + ", Max: " + max + ", Frags: " + frag;
                $('#heap').text(heapinfo);

                var resetReason = about.reason;
                var resetDescription = about.description;
                var resetText = "Reason: " + resetReason + ", Description: " + resetDescription;
                $('#resetreason').text(resetText);
            }
            catch {
                $('#uptime').text("(Error parsing uptime.)");
                $('#resetreason').text("(Error parsing reset reason.)");
                $('#heap').text("(Error parsing heap.)");
                $('#thisVersion').html("").html('<span class="text-danger">Error parsing version.</span>');
                $('#thisBranch').text();
                $('#thisBuild').text();
            }
        })
        .fail(function () {
            $('#uptime').text("(Error loading uptime.)");
            $('#heap').text("(Error loading heap.)");
            $('#resetreason').text("(Error parsing reset reason.)");
            $('#thisVersion').html("").html('<span class="text-danger">Error loading version.</span>');
            $('#thisBranch').text();
            $('#thisBuild').text();
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


function aboutReload() {
    loadAboutInfo(function callFunction() {
        setTimeout(aboutReload, aboutReloadTimer);
    });
}


function finishPage() { // Display page
    setTimeout(aboutReload, aboutReloadTimer);
}
