$(function () {
    $('[data-toggle="tooltip"]').tooltip()
});

// Attach the event after the page loads
if (window.addEventListener)
    window.addEventListener("load", startLoad, false);
else if (window.attachEvent)
    window.attachEvent("onload", startLoad);
else window.onload = startLoad;

function startLoad() { // Make sure the page is 100% loaded
    if (document.readyState === 'ready' || document.readyState === 'complete') {
        // loadMe();
    } else {
        document.onreadystatechange = function () {
            if (document.readyState == "complete") {
                // loadMe();
            }
        }
    }
}

function asleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

function getmdns(url) {
    return new Promise(function (resolve) {
        var xhr = new XMLHttpRequest();
        xhr.onload = function () {
            resolve(JSON.parse(xhr.responseText));
        };
        xhr.open('GET', url);
        xhr.overrideMimeType("application/json");
        xhr.send();
    });
}

var newmDNS;
getmdns('/settings/json/')
    .then(function (result) {
        newmDNS = result.mdnsID;
        //console.log(newmDNS);
    })

function redirect_now() {
    window.location.replace("http://" + newmDNS + ".local");
}

async function delayed_redirect() {
    await asleep(20000);
    window.location.replace("http://" + newmDNS + ".local");
}

delayed_redirect();