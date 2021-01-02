var posted = false;

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

var vm = new Vue({
    el: '#settingsapp',
    data: {
        settings: []
    },
    mounted: function () {
        this.getSettings();
    },
    methods: {
        getSettings: function () {
            var xhr = new XMLHttpRequest();
            var self = this;
            xhr.open('GET', '/settings/json/');
            xhr.onload = function () {
                self.settings = JSON.parse(xhr.responseText);
                if (self.settings.all_valid == "0") window.alert("Invalid value for one or more parameters!  Please confirm settings and resubmit");
                if (self.settings.invertTFT) document.getElementById("invertTFT").checked = true;
                if (self.settings.applyCalibration) document.getElementById("applyCalibration").checked = true;
                if (self.settings.tempCorrect) document.getElementById("tempCorrect").checked = true;
                document.getElementById("tempUnit").value = self.settings.tempUnit;
            };
            xhr.send()
        }
    },
    updated: function () {
        this.$nextTick(function () {
            // Code that will run only after the
            // entire view has been re-rendered
            posted = true;
        })
    }
});

function buttonDisable() {
    posted = false;
    $("button[id='submitSettings']").prop('disabled', true);
    $("button[id='submitSettings']").html('<i class="fa fa-spinner fa-spin"></i> Updating');
    $("button[id='resetWiFi']").prop('disabled', true);
    $("button[id='resetWiFi']").html('<i class="fa fa-spinner fa-spin"></i> Updating');
    buttonClearDelay();
}

function buttonClearDelay() { // Poll to see if entire page is loaded
    if (posted) {
        $("button[id='submitSettings']").prop('disabled', false);
        $("button[id='submitSettings']").html('Update');
        $("button[id='resetWiFi']").prop('disabled', false);
        $("button[id='resetWiFi']").html('Reset WiFi');
        posted = false;
    } else {
        setTimeout(buttonClearDelay, 500); // try again in 300 milliseconds
    }
}
