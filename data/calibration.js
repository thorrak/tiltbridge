// Attach the event after the page loads
if (window.addEventListener)
    window.addEventListener("load", startLoad, false);
else if (window.attachEvent)
    window.attachEvent("onload", startLoad);
else window.onload = startLoad;

function startLoad() { // Make sure the page is 100% loaded
    if (document.readyState === 'ready' || document.readyState === 'complete') {
        loaded();
    } else {
        document.onreadystatechange = function () {
            if (document.readyState == "complete") {
                loaded();
            }
        }
    }
}

$(function () {
    $('[data-toggle="tooltip"]').tooltip()
});

/* Regression.JS */
/* the librar is embedded for your convenience .*/
/**
 * @license
 *
 * Regression.JS - Regression functions for javascript
 * http://tom-alexander.github.com/regression-js/
 *
 * copyright(c) 2013 Tom Alexander
 * Licensed under the MIT license.
 *
 * @module regression - Least-squares regression functions for JavaScript
 **/
! function (a, b) {
    var c;
    return c = "function" == typeof define && define.amd ? define("regression", b) : "undefined" != typeof module ? module.exports = b() : a.regression = b()
}(this, function () {
    "use strict";

    function a(a, b) {
        var c = a.reduce(function (a, b) {
            return a + b[1]
        }, 0),
            d = c / a.length,
            e = a.reduce(function (a, b) {
                var c = b[1] - d;
                return a + c * c
            }, 0),
            f = a.reduce(function (a, c, d) {
                var e = b[d],
                    f = c[1] - e[1];
                return a + f * f
            }, 0);
        return 1 - f / e
    }

    function b(a, b) {
        var c = 0,
            d = 0,
            e = 0,
            f = 0,
            g = 0,
            h = a.length - 1,
            i = new Array(b);
        for (c = 0; h > c; c++) {
            for (f = c, d = c + 1; h > d; d++) Math.abs(a[c][d]) > Math.abs(a[c][f]) && (f = d);
            for (e = c; h + 1 > e; e++) g = a[e][c], a[e][c] = a[e][f], a[e][f] = g;
            for (d = c + 1; h > d; d++)
                for (e = h; e >= c; e--) a[e][d] -= a[e][c] * a[c][d] / a[c][c]
        }
        for (d = h - 1; d >= 0; d--) {
            for (g = 0, e = d + 1; h > e; e++) g += a[e][d] * i[e];
            i[d] = (a[h][d] - g) / a[d][d]
        }
        return i
    }

    function c(a, b) {
        var c = Math.pow(10, b);
        return Math.round(a * c) / c
    }
    var d, e = 2,
        f = {
            polynomial: function (d, e, f) {
                var g, h, i, j, k, l, m, n, o = [],
                    p = [],
                    q = 0,
                    r = 0,
                    s = d.length;
                for (h = "undefined" == typeof e ? 3 : e + 1, i = 0; h > i; i++) {
                    for (k = 0; s > k; k++) null !== d[k][1] && (q += Math.pow(d[k][0], i) * d[k][1]);
                    for (o.push(q), q = 0, g = [], j = 0; h > j; j++) {
                        for (k = 0; s > k; k++) null !== d[k][1] && (r += Math.pow(d[k][0], i + j));
                        g.push(r), r = 0
                    }
                    p.push(g)
                }
                for (p.push(o), m = b(p, h), l = d.map(function (a) {
                    var b = a[0],
                        c = m.reduce(function (a, c, d) {
                            return a + c * Math.pow(b, d)
                        }, 0);
                    return [b, c]
                }), n = "y = ", i = m.length - 1; i >= 0; i--) n += i > 1 ? c(m[i], f.precision) + "x^" + i + " + " : 1 === i ? c(m[i], f.precision) + "x + " : c(m[i], f.precision);
                return {
                    r2: a(d, l),
                    equation: m,
                    points: l,
                    string: n,
                    x0: c(m[0], f.precision),
                    x1: c(m[1], f.precision),
                    x2: c(m[2], f.precision),
                    x3: c(m[3], f.precision)
                }
            },
        };
    return d = function (a, b, c, d) {
        var g = "object" == typeof c && "undefined" == typeof d ? c : d || {};
        return g.precision || (g.precision = e), "string" == typeof a ? f[a.toLowerCase()](b, c, g) : null
    }
});
/*** end of Regression.JS **/

function C2F(c) {
    return (c * 1.8 + 32);
}

var BrewMath = {
    // these formula are taken from https://straighttothepint.com/specific-gravity-brix-plato-conversion-calculators/
    // and seem to the the most accurate
    plato2sg: function (plato) {
        return 1.0 + (plato / (258.6 - ((plato / 258.2) * 227.1)));
    },
    sgTempCorrectedC: function (Reading, C, CT) {
        return BrewMath.sgTempCorrected(Reading, C2F(C), C2F(CT));
    },
    sgTempCorrected: function (Reading, F, CT) {
        return Reading * ((1.00130346 - 0.000134722124 * F + 0.00000204052596 * F * F - 0.00000000232820948 * F * F * F) / (1.00130346 - 0.000134722124 * CT + 0.00000204052596 * CT * CT - 0.00000000232820948 * CT * CT * CT));
    },
    brix2sg: function (brix) {
        return (brix / (258.6 - ((brix / 258.2) * 227.1))) + 1;
    }
};


function applyTempCal() {
    var cal = parseFloat($("#tempcalvalue").val());
    if (isNaN(cal)) {
        alert("invalid calibration value");
        return;
    }
    s_ajax({
        url: tcurl,
        m: "POST",
        data: "tc=" + cal,
        success: function (d) {
            alert("done");
        },
        fail: function (d) {
            alert("failed:" + d);
        }
    });
}

function sgchanged() {
    var gravinput = parseFloat($("#gravinput").val());
    if (isNaN(gravinput)) return;

    // Convert Plato to SG
    if ($("#gu_select").val() == "gu_plato") {
        sg = BrewMath.plato2sg(gravinput);
    } else if ($("#gu_select").val() == "gu_brix") {
        sg = BrewMath.brix2sg(gravinput);
    } else {
        sg = gravinput;
    }

    if ($("#instrument").val() == "hydro") {
        var temp = parseFloat($("#worttemp").val());
        if (isNaN(temp)) return;
        var tc = $("#tempcorrect").prop("checked");
        var ct = $("#sgcorrect").val().trim();
        var useF = window.temp_unit == "F";
        var calsg = tc ? (useF ? BrewMath.sgTempCorrected(sg, temp, ct) : BrewMath.sgTempCorrectedC(sg, temp, ct)) : sg;
        document.getElementById('sg_result').value = calsg.toFixed(5)
    } else {
        var wc = parseFloat($("#wortcorrect").val())
        if (isNaN(wc)) return;
        var calsg = sg / parseFloat(wc);
        document.getElementById('sg_result').value = calsg.toFixed(5)
    }
}

var TiltPoints = {
    points: [],
    clear: function () {
        var tbody = document.getElementById("pointlist").getElementsByTagName("tbody")[0];
        var rl = tbody.getElementsByTagName("tr");
        var count = rl.length;
        for (var i = rl.length - 1; i > 0; i--) {
            var tr = rl[i];
            tr.parentNode.removeChild(tr);
        }
        return tbody;
    },
    del: function (i) {
        this.points.splice(i, 1);
        this.list();
    },
    newrow: function (i) {
        var tr = document.createElement("tr");
        var td1 = document.createElement("td");
        td1.className = "pl_del";
        var db = document.createElement("button");
        db.appendChild(document.createTextNode("Delete"));
        db.classList.add('btn');
        db.classList.add('btn-primary');
        db.onclick = function () {
            TiltPoints.del(i);
        };
        td1.appendChild(db);

        var td2 = document.createElement("td");
        td2.className = "pl_tilt";
        td2.innerHTML = this.points[i][0];
        var td3 = document.createElement("td");

        var precision = 5;

        var td4 = document.createElement("td");
        td4.className = "pl_sg";
        td4.innerHTML = this.points[i][1].toFixed(precision);

        var tdfi = document.createElement("td");
        tdfi.className = "pl_1stvalue";
        tdfi.innerHTML = (typeof window.firstValue != "undefined") ? window.firstValue[i].toFixed(precision) : "-";

        var tdf = document.createElement("td");
        tdf.className = "pl_1sterror";
        tdf.innerHTML = (typeof window.firstError != "undefined") ? window.firstError[i].toFixed(precision) : "-";


        var td5i = document.createElement("td");
        td5i.className = "pl_2ndvalue";
        td5i.innerHTML = (typeof window.secValue != "undefined") ? window.secValue[i].toFixed(precision) : "-";

        var td5 = document.createElement("td");
        td5.className = "pl_2nderror";
        td5.innerHTML = (typeof window.secondError != "undefined") ? window.secondError[i].toFixed(precision) : "-";

        var td6i = document.createElement("td");
        td6i.className = "pl_3rdvalue";
        td6i.innerHTML = (typeof window.thirdValue != "undefined") ? window.thirdValue[i].toFixed(precision) : "-";


        var td6 = document.createElement("td");
        td6.className = "pl_3rderror";
        td6.innerHTML = (typeof window.thirdError != "undefined") ? window.thirdError[i].toFixed(precision) : "-";

        tr.appendChild(td2);
        tr.appendChild(td4);
        tr.appendChild(tdfi);
        tr.appendChild(tdf);
        tr.appendChild(td5i);
        tr.appendChild(td5);
        tr.appendChild(td6i);
        tr.appendChild(td6);
        tr.appendChild(td1);
        return tr;
    },
    list: function () {
        this.regression();
        var tbody = this.clear();
        for (var i = 0; i < this.points.length; i++) {
            tbody.appendChild(this.newrow(i));
        }
    },
    regression: function () {
        if (this.points.length < 2) {
            $("#linear_polynomial").hide();
            $("#quadratic_polynomial").hide();
            $("#cubic_polynomial").hide();
            $("#graph").hide();
            return;
        }
        var firstRegression = regression('polynomial', this.points, 1, {
            precision: 5
        });
        var secondRegression, thirdRegression;
        if (this.points.length > 2) {
            secondRegression = regression('polynomial', this.points, 2, {
                precision: 5
            });
        }
        if (this.points.length > 3) {
            thirdRegression = regression('polynomial', this.points, 3, {
                precision: 5
            });
        }

        $("#quadratic_polynomial").hide();
        $("#cubic_polynomial").hide();
        document.getElementById('linearFitx0').value = firstRegression.x0;
        document.getElementById('linearFitx1').value = firstRegression.x1;
        $("#linear_polynomial").show();
        fit = firstRegression.string;

        if (this.points.length > 2) {
            $("#linear_polynomial").hide();
            $("#cubic_polynomial").hide();
            document.getElementById('quadraticFitx0').value = secondRegression.x0;
            document.getElementById('quadraticFitx1').value = secondRegression.x1;
            document.getElementById('quadraticFitx2').value = secondRegression.x2;
            $("#quadratic_polynomial").show();
            fit = secondRegression.string;

        }

        if (this.points.length > 3) {
            $("#linear_polynomial").hide();
            $("#quadratic_polynomial").hide();
            document.getElementById('cubicFitx0').value = thirdRegression.x0;
            document.getElementById('cubicFitx1').value = thirdRegression.x1;
            document.getElementById('cubicFitx2').value = thirdRegression.x2;
            document.getElementById('cubicFitx3').value = thirdRegression.x3;
            $("#cubic_polynomial").show();
            fit = thirdRegression.string;
        }

        $("#graph").show();
        functionPlot({
            target: '#graph',
            xAxis: { label: 'Uncalibrated Readings (SG)', domain: [0.97, 1.2] },
            yAxis: { label: 'Calibrated Readings (SG)', domain: [0.97, 1.2] },
            data: [
                { fn: fit },
                { points: this.points, fnType: 'points', graphType: 'scatter' }
            ]
        });

        // caluate errors
        var secoe, thridoe;

        var first_error = [];
        var first_value = [];
        var firstoe = firstRegression.equation;

        var sec_error = [];
        var sec_value = [];
        if (this.points.length > 2) secoe = secondRegression.equation;

        var third_error = [];
        var third_value = [];
        if (this.points.length > 3) thirdcoe = thirdRegression.equation;

        for (var i = 0; i < this.points.length; i++) {
            var x = this.points[i][0];
            var x2 = x * x;
            var x3 = x2 * x;
            var y = this.points[i][1]

            var f = firstoe[0] + firstoe[1] * x;
            first_error.push(f - y);
            first_value.push(f);

            if (this.points.length > 2) {
                var s = secoe[0] + secoe[1] * x + secoe[2] * x2;
                sec_error.push(s - y);
                sec_value.push(s);
            }

            if (this.points.length > 3) {
                var t = thirdcoe[0] + thirdcoe[1] * x + thirdcoe[2] * x2 + thirdcoe[3] * x3;
                third_error.push(t - y);
                third_value.push(t);
            }
        }
        window.firstError = first_error;
        window.equation1st = firstoe;
        window.firstValue = first_value;
        if (this.points.length > 2) {
            window.secondError = sec_error;
            window.equation2nd = secoe;
            window.secValue = sec_value;
        } else if (typeof window.secondError != "undefined") {
            delete (window.secondError);
            delete (window.equation2nd);
            delete (window.secValue);
        }

        if (this.points.length > 3) {
            window.equation3rd = thirdcoe;
            window.thirdValue = third_value;
            window.thirdError = third_error;
        } else if (typeof window.equation3rd != "undefined") {
            delete (window.equation3rd);
            delete (window.thirdValue);
            delete (window.thirdError);
        }

    },
    adddata: function () {
        var tilt = parseFloat($("#tiltinput").val());
        var gravity = parseFloat($("#sg_result").val());
        if (isNaN(tilt) || isNaN(gravity)) {
            alert("Cannot add invalid values");
            return;
        }
        this.points.push([tilt, gravity]);
        this.points.sort(function (a, b) {
            return a[0] - b[0];
        });
        this.list();
    },
};

function temperatureunit(celisus) {
    window.temp_unit = celisus ? "C" : "F";
}

function tempunitchange() {
    temperatureunit($("#tu_select").val() == "C");
}

function instrument(hydro) {
    if (hydro) {
        $("#hydrometer_input").show();
        $("#refractometer_input").hide();
    } else {
        $("#hydrometer_input").hide();
        $("#refractometer_input").show();
    }
}

function instrumentchange() {
    instrument($("#instrument").val() == "hydro");
}

function loaded() {
    //default to brix and refractometer
    instrument(false);
    $("#linear_polynomial").hide();
    $("#quadratic_polynomial").hide();
    $("#cubic_polynomial").hide();
    $("#graph").hide();
}

function buttonDisable() {
    // All this is going to do is disable the buttons, the whole page
    // (currently) reloads.  The work we do in settings is a little
    // more glamorous.
    $("button[id='clearSettings']").prop('disabled', true);
    $("button[id='clearSettings']").html('<i class="fa fa-spinner fa-spin"></i> Updating');
    $("button[id='applySettings']").prop('disabled', true);
    $("button[id='applySettings']").html('<i class="fa fa-spinner fa-spin"></i> Updating');
}
