// Attach the event after the page loads
if (window.addEventListener)
    window.addEventListener("load", loadPage, false);
else if (window.attachEvent)
    window.attachEvent("onload", loadPage);
else window.onload = loadPage;

function loadPage() { // Make sure the page is 100% loaded
    if(document.readyState === 'ready' || document.readyState === 'complete') {
        doResetSignal();
    } else {
        document.onreadystatechange = function () {
            if (document.readyState == "complete") {
                doResetSignal();
            }
        }
    }
}

function doResetSignal() {
    var jqxhr = $.get( "/resetwifi/")
        .done(function() {
            //
        })
        .fail(function() {
            // Reset is complete
            $("#card-title").replaceWith("<h4 class='card-header' class='card-title'>WiFi Reset Failed; Redirect Pending</h4>");
            $("#card-body").replaceWith("<p class='card-body'>The WiFi reset failed. You will be redirected momentarily.</p>");
            setTimeout(function () { window.location.href = "/"; }, 5000);
        });
}
