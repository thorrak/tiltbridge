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
                        cardData['textClass']='text-' + self.fullDict[key].color.toLowerCase() + '-bg';
                        cardData['bgClass']='bg-' + self.fullDict[key].color.toLowerCase();
                        cardData['borderClass']='border-' + self.fullDict[key].color.toLowerCase();
                        self.sensors.push(cardData);
                        //console.log(cardData);
                    });
                }
            };
            xhr.send()
        }
    }
});
