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
    }
});
