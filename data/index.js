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
        // populateConfig();
    } else {
        document.onreadystatechange = function () {
            if (document.readyState == "complete") {
                // populateConfig();
            }
        }
    }
}

var vm = new Vue({
    el: '#gravapp',
    data: {
        sensors: []
    },
    mounted: function () {
        this.getSensors();
        window.setInterval(() => {
            this.getSensors()
        }, 10000)  // Extending the delay as we don't update that often
    },
    methods: {
        getSensors: function() {
            var xhr = new XMLHttpRequest();
            var self = this;
            xhr.open('GET', '/json/');
            xhr.onload = function () {
                self.fullDict = JSON.parse(xhr.responseText);
                self.sensors = [];

                if (self.fullDict != null) {
                    Object.keys(self.fullDict).forEach(function(key) {
                        cardData = self.fullDict[key];

                        // Style
                        cardData['bgClass'] = 'bg-' + self.fullDict[key].color.toLowerCase();

                        // Only show Battery data if present
                        if (cardData['sends_battery']) {
                            cardData['battery'] = "Battery Age: " + cardData['weeks_on_battery'];
                        }

                        // Switch between Tilt and Tilt Pro title as appropriate
                        if (cardData['high_resolution']) {
                            cardData['title'] = "Tilt Pro";
                        } else {
                            cardData['title'] = "Tilt";
                        }

                        // Make a graceful landing if we have no GSheets URL
                        if (cardData['gsheets_link']) {
                            cardData['glink'] = cardData['gsheets_link']
                        } else {
                            cardData['glink'] = "/gsheets/"
                        }

                        self.sensors.push(cardData);
                    });
                }
            };
            xhr.send()
        }
    }
});
