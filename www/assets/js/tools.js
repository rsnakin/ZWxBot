var currentPage = "dashboard";
var tempChart;
var pressureChart;
var humidityChart;
var reloadRTChartsInterval;
var reloadLogInterval;
var reloadDashboardInterval;
var lastStringNumber = 0;
const logInterval = 60000;
const chartsInterval = 60000;
const dashboardInterval = 30000;
const totalDuration = 100;
const delayBetweenPoints = totalDuration / 2880;
const previousY = (ctx) => ctx.index === 0 ? ctx.chart.scales.y.getPixelForValue(100) : ctx.chart.getDatasetMeta(ctx.datasetIndex).data[ctx.index - 1].getProps(['y'], true).y;

function getCookie(name) {
    const value = `; ${document.cookie}`;
    const parts = value.split(`; ${name}=`);
    if (parts.length === 2) return parts.pop().split(';').shift();
}
function reloadRTCharts() {
    $.getJSON('/api/temp_data', function(data) {
        const tempData = [];
        const perssureData = [];
        const humidityData = [];
        let lastRecord = data[data.length - 1].t0;
        $('#cData').text(lastRecord);
        const now = new Date(lastRecord);
        data.forEach(function(item, index) {
            const targetTime = new Date(item.t0);
            let nowSecs = Math.floor(now.getTime() / 1000);
            let targetSecs = Math.floor(targetTime.getTime() / 1000);
            if(nowSecs - targetSecs <= 90000) {
                let tTime = (targetTime - now) / 3600000;
                tempData.push({x: tTime, y: item.v1});
                perssureData.push({x: tTime, y: item.v2 * 0.75006375541921 / 100.});
                humidityData.push({x: tTime, y: item.v3});
            }
        });
        const ctxTemp = document.getElementById('chartTemp');
        const ctxPressure = document.getElementById('chartPressure');
        const ctxHumidity = document.getElementById('chartHumidity');
        const configHumidity = {
            type: 'line',
            data: {
                datasets: [
                    {
                        borderColor: "rgb(149, 84, 0)",
                        borderWidth: 1,
                        radius: 0,
                        data: humidityData,
                    }
                ]
            },
            options: {
                responsive:true,
                maintainAspectRatio: false,
                interaction: {
                    intersect: false
                },
                plugins: {
                    legend: false,
                    title: {
                        display: true,
                        text: 'Humidity (DHT11)',
                        font: {
                            size: 20,
                            family: 'Nunito',
                            style: 'normal',
                            weight: 'bold'
                        },
                        color: '#000'
                    },
                    tooltip: {
                        displayColors: false,
                        callbacks: {
                            title: (tooltipItems) => {
                                const hoursAgo = tooltipItems[0].parsed.x;
                                const now = new Date(lastRecord);
                                const shifted = new Date(now.getTime() + hoursAgo * 3600 * 1000);  // hoursAgo < 0
                                const hours = String(shifted.getHours()).padStart(2, '0');
                                const minutes = String(shifted.getMinutes()).padStart(2, '0');
                                const seconds = String(shifted.getSeconds()).padStart(2, '0');
                                return `${hours}:${minutes}:${seconds}`;
                            },
                            label: context => context.formattedValue + ' %'
                        }
                    }
                },
                scales: {
                    x: {
                        type: 'linear',
                        ticks: {
                            callback: function(value) {
                                const hoursAgo = value;
                                const now = new Date(lastRecord);
                                const shifted = new Date(now.getTime() + hoursAgo * 3600 * 1000);
                                const hours = String(shifted.getHours()).padStart(2, '0');
                                const minutes = String(shifted.getMinutes()).padStart(2, '0');
                                const seconds = String(shifted.getSeconds()).padStart(2, '0');
                                return `${hours}:${minutes}:${seconds}`;
                            }
                        }
                    },
                    y: {
                        ticks: {
                            callback: function(value) {
                                return value + ' %';
                            }
                        }
                    }                    }
            }
        };
        const configPressure = {
            type: 'line',
            data: {
                datasets: [
                    {
                        borderColor: "rgb(0, 56, 134)",
                        borderWidth: 1,
                        radius: 0,
                        data: perssureData,
                    }
                ]
            },
            options: {
                responsive:true,
                maintainAspectRatio: false,
                interaction: {
                    intersect: false
                },
                plugins: {
                    legend: false,
                    title: {
                        display: true,
                        text: 'Pressure (BMP180)',
                        font: {
                            size: 20,
                            family: 'Nunito',
                            style: 'normal',
                            weight: 'bold'
                        },
                        color: '#000'
                    },
                    tooltip: {
                        displayColors: false,
                        callbacks: {
                            title: (tooltipItems) => {
                                const hoursAgo = tooltipItems[0].parsed.x;
                                const now = new Date(lastRecord);
                                const shifted = new Date(now.getTime() + hoursAgo * 3600 * 1000);
                                const hours = String(shifted.getHours()).padStart(2, '0');
                                const minutes = String(shifted.getMinutes()).padStart(2, '0');
                                const seconds = String(shifted.getSeconds()).padStart(2, '0');
                                return `${hours}:${minutes}:${seconds}`;
                            },
                            label: ({ parsed }) => parsed.y.toFixed(1) + ' mmHg'
                        }
                    }
                },
                scales: {
                    x: {
                        type: 'linear',
                        ticks: {
                            callback: function(value) {
                                const hoursAgo = value;
                                const now = new Date(lastRecord);
                                const shifted = new Date(now.getTime() + hoursAgo * 3600 * 1000);
                                const hours = String(shifted.getHours()).padStart(2, '0');
                                const minutes = String(shifted.getMinutes()).padStart(2, '0');
                                const seconds = String(shifted.getSeconds()).padStart(2, '0');
                                return `${hours}:${minutes}:${seconds}`;
                            }
                        }
                    }
                }
            }
        };
        const configTemp = {
            type: 'line',
            data: {
                datasets: [
                    {
                        borderColor: "rgb(7, 138, 0)",
                        borderWidth: 1,
                        radius: 0,
                        data: tempData,
                    }
                ]
            },
            options: {
                responsive:true,
                maintainAspectRatio: false,
                interaction: {
                    intersect: false
                },
                plugins: {
                    legend: false,
                    title: {
                        display: true,
                        text: 'Temperature (DS18B20)',
                        font: {
                            size: 20,
                            family: 'Nunito',
                            style: 'normal',
                            weight: 'bold'
                        },
                        color: '#000'
                    },
                    tooltip: {
                        displayColors: false,
                        callbacks: {
                            title: (tooltipItems) => {
                                const hoursAgo = tooltipItems[0].parsed.x;
                                const now = new Date(lastRecord);
                                const shifted = new Date(now.getTime() + hoursAgo * 3600 * 1000);
                                const hours = String(shifted.getHours()).padStart(2, '0');
                                const minutes = String(shifted.getMinutes()).padStart(2, '0');
                                const seconds = String(shifted.getSeconds()).padStart(2, '0');
                                return `${hours}:${minutes}:${seconds}`;
                            },
                            label:({ parsed }) => parsed.y.toFixed(1) + ' °C'
                        }
                    }
                },
                scales: {
                    x: {
                        type: 'linear',
                        ticks: {
                            callback: function(value) {
                                const hoursAgo = value;
                                const now = new Date(lastRecord);
                                const shifted = new Date(now.getTime() + hoursAgo * 3600 * 1000);  // hoursAgo < 0
                                const hours = String(shifted.getHours()).padStart(2, '0');
                                const minutes = String(shifted.getMinutes()).padStart(2, '0');
                                const seconds = String(shifted.getSeconds()).padStart(2, '0');
                                return `${hours}:${minutes}:${seconds}`;
                            }
                        }
                    },
                    y: {
                        ticks: {
                            callback: function(value) {
                                return value + ' °C';
                            }
                        }
                    }
                }
            }
        };
        if(!tempChart) {
            tempChart = new Chart(ctxTemp, configTemp);
        } else {
            tempChart.data.datasets[0].data = tempData;
            tempChart.options = configTemp.options;
            tempChart.update();
        }
        if(!pressureChart) {
            pressureChart = new Chart(ctxPressure, configPressure);
        } else {
            pressureChart.data.datasets[0].data = perssureData;
            pressureChart.options = configPressure.options;
            pressureChart.update();
        }
        if(!humidityChart) {
            humidityChart = new Chart(ctxHumidity, configHumidity);
        } else {
            humidityChart.data.datasets[0].data = humidityData;
            humidityChart.options = configHumidity.options;
            humidityChart.update();
        }
    });
}
function addModalWindow() {
    var modalWindow = '<div class="modal fade" role="dialog" tabindex="-1" id="modal-1">' +
        '<div class="modal-dialog" role="document"><div class="modal-content">' +
        '<div class="modal-header text-white bg-gradient-primary">' +
        '<h4 class="modal-title" id="modTitle"></h4><button type="button" class="btn-close" ' +
        'data-bs-dismiss="modal" aria-label="Close" id="modalBClose"></button></div>' +
        '<div class="modal-body" style="text-align:center;border-style: none;">' +
        '<p id="modContent"></p></div>' +
        '<div class="modal-footer" style="border-style: none;">' +
        '<button id="modalClose" class="btn btn-light bg-gradient-primary text-white" type="button" ' +
        'data-bs-dismiss="modal">Close</button></div>' +
        '</div></div></div>';
    $('body').append(modalWindow);
}
function rtChartsFunc() {
    let html = '<div class="container-fluid">' +
        '<div id="topic_id" class="d-sm-flex justify-content-between align-items-center mb-4">' +
        '<h3 class="text-dark mb-0" style="padding: 0px;padding-top:10px;font-weight:bold;">' +
        '<i class="fas fa-chart-line"></i> Real-time charts: <span id="cData"></span></h3></div>' +
        '<div class="chart-area"><canvas id="chartTemp"></canvas></div>' +
        '<div class="chart-area"><canvas id="chartPressure"></canvas></div>' +
        '<div class="chart-area"><canvas id="chartHumidity"></canvas></div></div>';
    $('#currentContent').html(html);
    $('.chart-area').each(function () {
        $(this).height(parseInt(($(window).height() - $('#topic_id').outerHeight(true)) / 3));
    });
    reloadRTCharts();
    reloadRTChartsInterval = setInterval(() => {
        reloadRTCharts();
    }, chartsInterval);
}
function clearTRChart() {
    clearInterval(reloadRTChartsInterval);
    delete tempChart;
    tempChart = null;
    delete pressureChart;
    pressureChart = null;
    delete humidityChart;
    humidityChart = null;
}
function options() {
    let html = '<div class="container-fluid"><div id="opts_topic_id" class="d-sm-flex ' +
        'justify-content-between align-items-center mb-4">' +
        '<h3 class="text-dark mb-0" style="padding: 0px;padding-top:10px;font-weight:bold;">' +
        '<i class="fas fa-sliders-h"></i> Options</h3></div>' +
        '<div class="table-responsive"><table width="100%"><tbody>' +
        '<tr><td colspan="3">&nbsp;</td></tr><tr>' +
        '<td class="fw-bold" style="text-align: right;width: 50%;padding: 8px;padding-top: 11px;">' +
        'Owner chat id &#128100;</td>' +
        '<td style="width: 50%;"><input class="rounded-pill" type="text" ' +
        'placeholder="Owner chat id" name="ownerChatId" id="ownerChatId"></td></tr><tr>' +
        '<td class="fw-bold" style="text-align: right;width: 50%;padding: 8px;padding-top: 11px;">' +
        'Temperature Threshold 🌡</td>' +
        '<td style="width: 50%;"><input class="rounded-pill" type="text" id="temperatureThreshold" ' +
        'placeholder="Temperature Threshold" name="temperatureThreshold"></td></tr><tr>' +
        '<td class="fw-bold" style="text-align:right;width:50%;padding:8px;padding-top:11px;">' +
        'Humidity Threshold 💧</td>' +
        '<td style="width: 50%;"><input class="rounded-pill" type="text" id="humidityThreshold" ' +
        'placeholder="Humidity Threshold" name="humidityThreshold"></td></tr><tr>' +
        '<td class="fw-bold" style="text-align:right;width:50%;padding:8px;padding-top:11px;">' +
        'Pressure Threshold 🫧</td>' +
        '<td style="width: 50%;"><input class="rounded-pill" type="text" id="pressureThreshold" ' +
        'placeholder="Pressure Threshold" name="pressureThreshold"></td></tr><tr>' +
        '<td class="fw-bold" style="text-align:right;width:50%;padding:8px;padding-top:11px;">' +
        'Humidity correction 🛠️</td>' +
        '<td style="width: 50%;"><input class="rounded-pill" type="text" id="humidityCorrection" ' +
        'placeholder="Humidity correction" name="humidityCorrection"></td></tr><tr>' +
        '<td class="text-center" colspan="2" style="padding:35px;">' +
        '<button id="saveOptions" class="btn btn-primary bg-gradient-primary rounded-pill" type="button">' +
        'Save options</button></td></tr></tbody></table></div></div>';
    $('#currentContent').html(html);
    $.getJSON('/api/config', function(data) {
        $('#ownerChatId').val(data.ownerChatId);
        $('#temperatureThreshold').val(data.temperatureThreshold);
        $('#pressureThreshold').val(data.pressureThreshold);
        $('#humidityThreshold').val(data.humidityThreshold);
        $('#humidityCorrection').val(data.humidityCorrection);
    });
    $('#saveOptions').off('click');
    $('#saveOptions').click(function() {
        const myModal = new bootstrap.Modal(document.getElementById('modal-1'));
        $('#modTitle').text('');
        $('#modContent').html('Saving options...');
        $('#modalBClose').hide();
        $('#modalClose').hide();
        myModal.show();
        $.getJSON('/api/config_save', {
            ownerChatId: $('#ownerChatId').val(),
            temperatureThreshold: $('#temperatureThreshold').val(),
            pressureThreshold: $('#pressureThreshold').val(),
            humidityThreshold: $('#humidityThreshold').val(),
            humidityCorrection: $('#humidityCorrection').val()
        }, function(data) {
            $('#modTitle').html('&#x2755; Information');
            if(data.status == 'OK') {
                $('#modalBClose').show();
                $('#modalClose').text('Ok');
                $('#modalClose').show();
                $('#modContent').html('Saving options...Ok!');
            } else {
                $('#modalBClose').show();
                $('#modalClose').show();
                $('#modContent').html('Saving options...Error!');
            }
        });
    });
}
function getMoonIconByName(phaseName) {
    const moonPhases = {
        "Dark Moon": "🌑",
        "New Moon": "🌑",
        "Waxing Crescent": "🌒",
        "First Quarter": "🌓",
        "Waxing Gibbous": "🌔",
        "Full Moon": "🌕",
        "Waning Gibbous": "🌖",
        "Last Quarter": "🌗",
        "Waning Crescent": "🌘"
    };
    return moonPhases[phaseName] || "🌙";
}
function loadDashboard() {
    $.getJSON('/api/dashboard', function(data) {
        $('#tempValue').html('<b>' + data.temperature_a + data.temperature + ' °C</b>');
        $('#humidityValue').html('<b>' + data.humidity_a + data.humidity.toFixed(0) + ' %</b>');
        let pressure = data.pressure * 0.75006375541921 / 100.;
        $('#pressureValue').html('<b>' + data.pressure_a + pressure.toFixed(2) + ' mmHg</b>');
        $('#rain').html(data.isRain);
        $('#bmp180_temp').html('<b>' + data.bmp180_temp.toFixed(2) + ' °C</b>');
        $('#dht11_temp').html('<b>' + data.dht11_temp.toFixed(2) + '°C/' + 
            data.humidity_orig.toFixed(2) + '%</b>');
        $('#cpu').html('<b>' + data.cpuMHz.toFixed(0) + '/' + data.gpuMHz.toFixed(0) + ' MHz</b>');
        $('#ssid').html('<b>' + data.ssid + '</b>');
        $('#rssi').html('<b>' + data.rssi + ' dBm</b>');
        $('#cpuTemp').html('<b>' + data.cpuTemp.toFixed(2) + ' °C</b>');
        $('#fan').html('<b>' + data.fan + '</b>');
        $('#throttled').html('<b>' + data.throttled + '</b>');
        $('#version').html('<b>V' + data.version + '</b>');
        $('#dMode').html("<b>" + data.isSystemd.toUpperCase() + '</b>');
        $('#uptime').html('<b>' + data.uptime + '</b>');
        //$('#moon').html(getMoonIconByName(data.todayMoon.Phase));
        //$('#moonName').html('<b>' + data.todayMoon.Phase + '</b>');
        $('#sunSet').html('<b>' + data.sunSet.sunrise + '→' + data.sunSet.sunset + '</b>');
    });
}
function dashboard() {
    let html = '<div class="container-fluid"><div id="opts_topic_id" class="d-sm-flex ' +
        'justify-content-between align-items-center mb-4">' +
        '<h3 class="text-dark mb-0" style="padding: 0px;padding-top:10px;font-weight:bold;">' +
        '<i class="fas fa-table"></i> Dashboard</h3></div>' +
        '<div class="row g-4 justify-content-center">' +

        '<div class="col-12 col-sm-6 col-md-4">' +
        '<div class="widget border border-primary">' +
        '<div class="label">🌡️ Temperature</div>' +
        '<h2 id="tempValue">&nbsp;</h2>' +
        '</div></div>' +

        '<div class="col-12 col-sm-6 col-md-4">' +
        '<div class="widget border border-primary">' +
        '<div class="label">🫧 Pressure</div>' +
        '<h2 id="pressureValue">&nbsp;</h2>' +
        '</div></div>' +

        '<div class="col-12 col-sm-6 col-md-4">' +
        '<div class="widget border border-primary">' +
        '<div class="label">💧 Humidity</div>' +
        '<h2 id="humidityValue">&nbsp;</h2>' +
        '</div></div>' +

        '<div class="col-12 col-sm-6 col-md-4">' +
        '<div class="widget border border-primary">' +
        '<div class="label">🌦️ Rain</div>' +
        '<h2 id="rain">&nbsp;</h2>' +
        '</div></div>' +

        '<div class="col-12 col-sm-6 col-md-4">' +
        '<div class="widget border border-primary">' +
        '<div class="label">☀️ Sun Times</div>' +
        '<h2 id="sunSet">&nbsp;</h2>' +
        '</div></div>' +

        '<div class="col-12 col-sm-6 col-md-4">' +
        '<div class="widget border border-primary">' +
        '<div class="label"><span id="moon">🌕</span> Moon phase</div>' +
        '<h2 id="moonName">&nbsp;</h2>' +
        '</div></div>' +

        '<div class="col-12 col-sm-6 col-md-4">' +
        '<div class="widget border border-primary">' +
        '<div class="label">🌡️ Temperature BMP180</div>' +
        '<h2 id="bmp180_temp">&nbsp;</h2>' +
        '</div></div>' +

        '<div class="col-12 col-sm-6 col-md-4">' +
        '<div class="widget border border-primary">' +
        '<div class="label">🌡️💧 Temp/Humid DHT11</div>' +
        '<h2 id="dht11_temp">&nbsp;</h2>' +
        '</div></div>' +

        '<div class="col-12 col-sm-6 col-md-4">' +
        '<div class="widget border border-primary">' +
        '<div class="label">🔥 CPU temperature (fan:<span id="fan"></span>)</div>' +
        '<h2 id="cpuTemp">&nbsp;</h2>' +
        '</div></div>' +

        '<div class="col-12 col-sm-6 col-md-4">' +
        '<div class="widget border border-primary">' +
        '<div class="label">🕒 Uptime</div>' +
        '<h2 id="uptime">&nbsp;</h2>' +
        '</div></div>' +

        '<div class="col-12 col-sm-6 col-md-4">' +
        '<div class="widget border border-primary">' +
        '<div class="label">⚙️ CPU/GPU</div>' +
        '<h2 id="cpu">&nbsp;</h2>' +
        '</div></div>' +

        '<div class="col-12 col-sm-6 col-md-4">' +
        '<div class="widget border border-primary">' +
        '<div class="label">🛰️ SSID: <span id="ssid">&nbsp;</span></div>' +
        '<h2 id="rssi">&nbsp;</h2>' +
        '</div></div>' +

        '<div class="col-12 col-sm-6 col-md-4">' +
        '<div class="widget border border-primary">' +
        '<div class="label">🐌 Throttled</div>' +
        '<h2 id="throttled">&nbsp;</h2>' +
        '</div></div>' +

        '<div class="col-12 col-sm-6 col-md-4">' +
        '<div class="widget border border-primary">' +
        '<div class="label">🎚️ Mode</div>' +
        '<h2 id="dMode">&nbsp;</h2>' +
        '</div></div>' +

        '<div class="col-12 col-sm-6 col-md-4">' +
        '<div class="widget border border-primary">' +
        '<div class="label">🛠️ Version</div>' +
        '<h2 id="version">&nbsp;</h2>' +
        '</div></div>' +

        '</div></div>';
    $('#currentContent').html(html);
    loadDashboard();
    reloadDashboardInterval = setInterval(() => {
        loadDashboard();
    }, dashboardInterval);
}
function animateColor(element, startColor, endColor, duration = 1000) {
  const start = parseColor(startColor);
  const end = parseColor(endColor);
  const startTime = performance.now();

  function step(currentTime) {
    const progress = Math.min((currentTime - startTime) / duration, 1);
    const r = Math.round(start.r + (end.r - start.r) * progress);
    const g = Math.round(start.g + (end.g - start.g) * progress);
    const b = Math.round(start.b + (end.b - start.b) * progress);
    element.style.color = `rgb(${r}, ${g}, ${b})`;
    if (progress < 1) {
      requestAnimationFrame(step);
    }
  }

  requestAnimationFrame(step);
}
function parseColor(color) {
  const ctx = document.createElement("canvas").getContext("2d");
  ctx.fillStyle = color;
  const computed = ctx.fillStyle;

  if (computed.startsWith("rgb")) {
    const matches = computed.match(/\d+/g);
    if (!matches || matches.length < 3) {
      throw new Error(`Failed to parse color components from: ${color}`);
    }
    return {
      r: Number(matches[0]),
      g: Number(matches[1]),
      b: Number(matches[2])
    };
  } else if (computed.startsWith("#")) {
    let r = parseInt(computed.substr(1, 2), 16);
    let g = parseInt(computed.substr(3, 2), 16);
    let b = parseInt(computed.substr(5, 2), 16);
    return { r, g, b };
  } else {
    throw new Error(`Unknown color format: ${computed}`);
  }
}
function loadLog() {
    $.get('/api/log', function(data) {
        let lines = data.split(/\r?\n/);
        let output = '';
        let sNumber = 0;
        lines.forEach(line => {
            if(line.trim() != '') {
                sNumber ++;
                let newLine = '';
                if(sNumber > lastStringNumber && lastStringNumber != 0) {
                    newLine = ' newLine';
                }
                if (line.includes('error') || line.includes('Error') || (line.includes('Reset') && !line.includes('PANIC'))) {
                    output += '<span class="log-error' + newLine + '" id="string_' + sNumber + '">' + line + '</span>';
                } else if (line.includes('PID')) {
                    output += '<span class="log-info' + newLine + '" id="string_' + sNumber + '">' + line + '</span>';
                } else if (line.includes('killed')) {
                    output += '<span class="log-panic' + newLine + '" id="string_' + sNumber + '">' + line + '</span>';
                } else if (line.includes('chatID')) {
                    output += '<span class="log-chatid' + newLine + '" id="string_' + sNumber + '">' + line + '</span>';
                } else if (line.includes('started') || line.includes('updateAutoBrightness') || line.includes('Start')) {
                    output += '<span class="log-start' + newLine + '" id="string_' + sNumber + '">' + line + '</span>';
                } else if (line.includes('Config') || line.includes('SunriseSunset') || 
                    line.includes('MoonPhases') || line.includes('makeChart')) {
                    output += '<span class="log-telegramAlert' + newLine + '" id="string_' + sNumber + '">' + line + '</span>';
                } else {
                    output += '<span class="log-plain' + newLine + '" id="string_' + sNumber + '">' + line + '</span>';
                }
            }
        });
        $("#logOutput").html(output);
        let log = document.getElementById("logOutput");
        log.scrollTop = log.scrollHeight;
        if(lastStringNumber != sNumber && lastStringNumber != 0) {
            let originalColors = [];
            let originalWeight = [];
            $('.newLine').each(function(index, element) {
                let $el = $(element);
                originalColors[index] = $el.css('color');
                originalWeight[index] = $el.css('font-weight');
                $el.css('color', 'rgb(225, 0, 0)');
                $el.css('font-weight', '700');
                $el.css('transition', 'font-weight 1s linear');
            });
            setTimeout(function() {
                $('.newLine').each(function(index, element) {
                    animateColor(this, 'rgb(225, 0, 0)', originalColors[index], 1000);
                    $(element).css('font-weight', originalWeight[index]);
                });
                lastStringNumber = sNumber;
            }, 2000);
        } else {
            lastStringNumber = sNumber;
        }
    }).fail(function() {
        $("#logOutput").text("⚠️ Unable to load log file.");
    });
}
function log() {
    let html = '<div class="container-fluid"><div id="opts_topic_id" class="d-sm-flex ' +
        'justify-content-between align-items-center mb-4">' +
        '<h3 class="text-dark mb-0" style="padding: 0px;padding-top:10px;font-weight:bold;">' +
        '<i class="fas fa-scroll"></i> Log</h3></div>' +
        '' +
        '<pre id="logOutput">Loading log...</pre>' +
        '</div>';
    $('#currentContent').html(html);
    loadLog();
    reloadLogInterval = setInterval(() => {
        loadLog();
    }, logInterval);

}
function clearDashboard() {
    clearInterval(reloadDashboardInterval);
}
function clearLog() {
    clearInterval(reloadLogInterval);
}
$('body').show();
if(getCookie('sidebarToggle')) {
    $('#sidebarToggle').trigger('click');
}
$('#sidebarToggle').click(function(){
    if(getCookie('sidebarToggle')) {
        document.cookie = "sidebarToggle=; path=/; expires=Thu, 01 Jan 1970 00:00:00 UTC;";
    } else {
        document.cookie = "sidebarToggle=1; path=/; max-age=" + 60*60*24*365*10;
    }
});
$('#wrapper').height($(window).height());
$('.nav-link').each(function(index, element) {
    $(this).removeClass('active');
});
$('.nav-link').click(function(){
    $('.nav-link').each(function() {
        $(this).removeClass('active');
    });
    currentPage = $(this).attr('id');
    if(currentPage == 'trCharts') {
        clearTRChart();
        clearLog();
        clearDashboard();
        $(this).addClass('active');
        rtChartsFunc();
    } else if(currentPage == 'options') {
        clearTRChart();
        clearLog();
        clearDashboard();
        $(this).addClass('active');
        options();
    }  else if(currentPage == 'log') {
        clearTRChart();
        clearDashboard();
        clearLog();
        $(this).addClass('active');
        log();

    }  else if(currentPage == 'dashboard') {
        clearTRChart();
        clearLog();
        clearDashboard();
        $(this).addClass('active');
        dashboard();
    }
});
addModalWindow();
if(currentPage == 'trCharts') {
    $('#trCharts').addClass('active');
    rtChartsFunc();
} else if(currentPage == 'dashboard'){
    $('#dashboard').addClass('active');
    dashboard();
}
