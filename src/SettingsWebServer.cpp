#include "SettingsWebServer.h"
#include "AppConfig.h"
#include "Configuration.h"
#include "DataSpaceList.h"
#include "NeoPixelLED.h"
#include "WebClientHandler.h"
#include <ArduinoJson.h>
#include <freertos/task.h>

// Live-Statusliste aus main.cpp – wird für die LED-Wiederherstellung nach dem Blinken benötigt.
extern std::vector<SpaceStatusList> spaceStatusList;

// ---------------------------------------------------------------------------
// Static PROGMEM blocks — keep large, read-only text out of DRAM entirely.
// Each block is streamed once per request; the ESP32 never needs to hold
// more than one chunk in heap at the same time.
// ---------------------------------------------------------------------------

// Shared CSS (injected by htmlHeader).
static const char CSS[] PROGMEM = R"css(
body{font-family:sans-serif;margin:0;background:#111;color:#eee;}
nav{background:#1a1a1a;padding:10px 20px;border-bottom:2px solid #e02020;}
nav a{color:#1a7a3c;text-decoration:none;margin-right:16px;font-weight:bold;}
nav a:hover{color:#fff;text-decoration:underline;}
.container{max-width:700px;margin:30px auto;padding:0 16px;}
h1{color:#1a7a3c;}
h2{color:#f5a800;}
label{display:block;margin-top:14px;font-size:0.9em;color:#bbb;}
input[type=text],input[type=number],input[type=url]
{width:100%;padding:7px;background:#1e1e1e;color:#eee;border:1px solid #3a3a3a;
border-radius:4px;box-sizing:border-box;margin-top:4px;}
input:focus{outline:2px solid #1a7a3c;border-color:#1a7a3c;}
.btn{display:inline-block;margin-top:20px;padding:10px 24px;
border:none;border-radius:5px;cursor:pointer;font-size:1em;font-weight:bold;}
.btn-save{background:#1a7a3c;color:#fff;}
.btn-save:hover{background:#2d9e55;}
.btn-reset{background:#f5a800;color:#fff;margin-left:12px;}
.btn-reset:hover{background:#ffc030;}
.btn-settings{display:inline-block;margin-top:20px;padding:10px 24px;
background:#1a7a3c;color:#fff;border:none;border-radius:5px;cursor:pointer;
font-size:1em;font-weight:bold;text-decoration:none;}
.btn-settings:hover{background:#24a050;}
.msg{margin-top:14px;padding:10px;background:#0e2e0e;border-left:4px solid #1a7a3c;
border-radius:4px;color:#6fcf6f;}
.msg-reset{background:#2e0e0e;border-left:4px solid #f5a800;color:#f88;}
)css";

// JavaScript for /spacemap — large block, lives in flash.
static const char SPACEMAP_JS[] PROGMEM = R"rawjs(
<script>
var rowCount = 0; // set dynamically before this script runs

function buildRow(i, name, city, disabled) {
    var isEmpty = (name === '' && city === '');
    var rowStyle = isEmpty ? 'opacity:0.45;' : '';
    if (disabled) rowStyle += 'font-style:italic;';
    return '<tr id="row_'+i+'" draggable="true" ondragstart="onDragStart(event)" ondragover="onDragOver(event)" ondrop="onDrop(event)" ondragend="onDragEnd(event)" style="cursor:grab;'+rowStyle+'">'
        + '<td style="padding:4px;width:24px;color:#555;font-size:1.2em;text-align:center;cursor:grab" title="Drag to reorder">&#8597;</td>'
        + '<td style="padding:4px;text-align:center;white-space:nowrap"><input type="checkbox" class="dis-check" name="dis_'+i+'" title="Deaktivieren (LED bleibt aus)" onchange="onDisabledChange(this)"'+(disabled?' checked':'')+'>  <button type="button" class="btn-blink" onclick="blinkLed(this)" style="background:#1a4a7a;color:#fff;border:none;border-radius:4px;padding:4px 8px;margin-left:4px;cursor:pointer" title="Blink to locate">&#128294;</button><span class="space-status" style="display:inline-block;min-width:80px;margin-left:6px;font-size:0.82em;vertical-align:middle;color:#888">&#8987; ...</span></td>'
        + '<td style="padding:4px;text-align:center;color:#555;font-size:0.9em;min-width:36px"><input type="hidden" name="led_'+i+'" value="'+i+'">'+i+'</td>'
        + '<td style="padding:4px"><input type="text" name="name_'+i+'" value="'+name+'" placeholder="(leer)" style="width:100%;background:#1e1e1e;color:#eee;border:1px solid #3a3a3a;border-radius:4px;padding:4px" oninput="onNameOrCityInput(this)"></td>'
        + '<td style="padding:4px"><input type="text" name="city_'+i+'" value="'+city+'" placeholder="(leer)" style="width:100%;background:#1e1e1e;color:#eee;border:1px solid #3a3a3a;border-radius:4px;padding:4px" oninput="onNameOrCityInput(this)"></td>'
        + '</tr>';
}

function onNameOrCityInput(inp) {
    var row = inp.closest('tr');
    markRowDirty(row);
    var inputs = row.querySelectorAll('input[type=text]');
    var empty = inputs[0].value === '' && inputs[1].value === '';
    row.style.opacity = empty ? '0.45' : '';
}

function onDisabledChange(cb) {
    var row = cb.closest('tr');
    markRowDirty(row);
    applyDisabledStyle(row);
}

function applyDisabledStyle(row) {
    var cb = row.querySelector('.dis-check');
    var textInputs = row.querySelectorAll('input[type=text]');
    var empty = textInputs.length >= 2 && textInputs[0].value === '' && textInputs[1].value === '';
    var isDisabled = cb && cb.checked;
    row.style.fontStyle = isDisabled ? 'italic' : '';
    // Keep empty-row dimming intact; don't override it when not disabled.
    if (!isDisabled && !empty) {
        row.style.opacity = '';
    }
}

function markRowDirty(row) {
    row.style.background = '#0d1f0d';
    row.style.outline = '1px solid #1a7a3c';
    var msg = document.getElementById('saveMsg');
    if (msg) msg.style.display = 'none';
}

function doImport() {
    var raw = document.getElementById('importArea').value;
    // With position + optional disabled flag: { 2, "Name", "City"} or { 2, "Name", "City", 1}
    var rePos = /\{\s*(\d+)\s*,\s*"([^"]*)"\s*,\s*"([^"]*)"(?:\s*,\s*(\d+))?\s*\}/g;
    // Without position + optional disabled flag: { "Name", "City"} or { "Name", "City", 1}
    var reNoPos = /\{\s*"([^"]*)"\s*,\s*"([^"]*)"(?:\s*,\s*(\d+))?\s*\}/g;
    var m;
    var withPos = {};
    while ((m = rePos.exec(raw)) !== null) {
        withPos[parseInt(m[1], 10)] = { pos: parseInt(m[1], 10), name: m[2], city: m[3], disabled: m[4] === '1' };
    }
    var stripped = raw.replace(/\{\s*\d+\s*,\s*"[^"]*"\s*,\s*"[^"]*"(?:\s*,\s*\d+)?\s*\}/g, '');
    var noPos = [];
    while ((m = reNoPos.exec(stripped)) !== null) {
        noPos.push({ name: m[1], city: m[2], disabled: m[3] === '1' });
    }
    if (Object.keys(withPos).length === 0 && noPos.length === 0) {
        alert('Keine Eintr\u00e4ge gefunden.\nErwartetes Format:\n{ 2, "MuCCC", "Munich"}\noder: { "MuCCC", "Munich"}');
        return;
    }
    var tbody = document.getElementById('mapBody');
    var rows = Array.from(tbody.querySelectorAll('tr'));
    Object.values(withPos).forEach(function(p) {
        if (p.pos >= 0 && p.pos < rows.length) {
            var row = rows[p.pos];
            var inputs = row.querySelectorAll('input[type=text]');
            var disCb  = row.querySelector('.dis-check');
            if (inputs.length >= 2) {
                inputs[0].value = p.name;
                inputs[1].value = p.city;
                if (disCb) disCb.checked = p.disabled;
                applyDisabledStyle(row);
                row.style.opacity = (p.name === '' && p.city === '') ? '0.45' : row.style.opacity;
                markRowDirty(row);
            }
        }
    });
    var freeIdx = 0;
    noPos.forEach(function(p) {
        while (freeIdx < rows.length) {
            var row = rows[freeIdx];
            var inputs = row.querySelectorAll('input[type=text]');
            var disCb  = row.querySelector('.dis-check');
            freeIdx++;
            if (inputs.length >= 2 && inputs[0].value === '') {
                inputs[0].value = p.name;
                inputs[1].value = p.city;
                if (disCb) disCb.checked = p.disabled;
                applyDisabledStyle(row);
                markRowDirty(row);
                break;
            }
        }
    });
    document.getElementById('importArea').value = '';
}

var dragSrc = null;
function onDragStart(e) {
    dragSrc = e.currentTarget;
    e.dataTransfer.effectAllowed = 'move';
    e.dataTransfer.setData('text/plain', '');
    setTimeout(function() { dragSrc.style.opacity = '0.4'; }, 0);
}
function onDragOver(e) {
    e.preventDefault();
    e.dataTransfer.dropEffect = 'move';
    var target = e.currentTarget;
    if (target !== dragSrc) target.style.borderTop = '2px solid #1a7a3c';
}
function onDrop(e) {
    e.preventDefault();
    var target = e.currentTarget;
    target.style.borderTop = '';
    if (dragSrc && target !== dragSrc) {
        var tbody = document.getElementById('mapBody');
        var rows  = Array.from(tbody.querySelectorAll('tr'));
        var fromIdx = rows.indexOf(dragSrc);
        var toIdx   = rows.indexOf(target);
        if (fromIdx < toIdx) tbody.insertBefore(dragSrc, target.nextSibling);
        else                 tbody.insertBefore(dragSrc, target);
        // After drag: re-assign LED indices based on new row order (positions are fixed)
        Array.from(tbody.querySelectorAll('tr')).forEach(function(r, idx) {
            // Update the hidden led input and the display label
            var ledHidden = r.querySelector('input[type=hidden]');
            var ledDisplay = r.querySelector('td:nth-child(3)');
            if (ledHidden) ledHidden.value = idx;
            if (ledDisplay) {
                // Update text node (last child is the text)
                var tn = ledDisplay.lastChild;
                if (tn && tn.nodeType === 3) tn.nodeValue = idx;
            }
            markRowDirty(r);
        });
        reindexRows();
    }
}
function onDragEnd(e) {
    var row = e.currentTarget;
    var inputs = row.querySelectorAll('input[type=text]');
    var isEmpty = inputs.length >= 2 && inputs[0].value === '' && inputs[1].value === '';
    row.style.opacity = isEmpty ? '0.45' : '';
    document.querySelectorAll('#mapBody tr').forEach(function(r) { r.style.borderTop = ''; });
}

function doExport() {
    var rows = document.getElementById('mapBody').querySelectorAll('tr');
    var parts = [];
    rows.forEach(function(r) {
        var ledHidden = r.querySelector('input[type=hidden]');
        var textInputs = r.querySelectorAll('input[type=text]');
        var disCb = r.querySelector('.dis-check');
        var led      = ledHidden ? ledHidden.value : '0';
        var name     = textInputs.length > 0 ? textInputs[0].value : '';
        var city     = textInputs.length > 1 ? textInputs[1].value : '';
        var disabled = (disCb && disCb.checked) ? '1' : '0';
        parts.push('{ ' + led + ', "' + name + '", "' + city + '", ' + disabled + '}');
    });
    var blob = new Blob([parts.join(', ') + '\n'], { type: 'text/plain' });
    var a = document.createElement('a');
    a.href = URL.createObjectURL(blob);
    a.download = 'searchList.txt';
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(a.href);
}

function blinkLed(btn) {
    var ledHidden = btn.closest('tr').querySelector('input[type=hidden]');
    var ledIndex = ledHidden ? parseInt(ledHidden.value, 10) : 0;
    var orig = btn.innerHTML;
    btn.disabled = true;
    btn.innerHTML = '&#8987;';
    fetch('/spacemap/blink?led=' + ledIndex)
        .then(function(r){ return r.json(); })
        .then(function(d){
            btn.innerHTML = d.ok ? '&#10003;' : '&#10005;';
            setTimeout(function(){ btn.innerHTML = orig; btn.disabled = false; }, 1500);
        })
        .catch(function(){
            btn.innerHTML = '&#10005;';
            setTimeout(function(){ btn.innerHTML = orig; btn.disabled = false; }, 1500);
        });
}

// Last known ageSec from /api/status — used to detect when the ESP has
// fetched fresh data from the SpaceAPI so the UI can update immediately.
var _lastAgeSec = -1;

function updateSpaceStatus() {
    Promise.all([
        fetch('/api/spacestatus').then(function(r) { return r.json(); }),
        fetch('/api/status').then(function(r) { return r.json(); })
    ])
    .then(function(results) {
        var list      = results[0];
        var apiStatus = results[1];

        // --- Detect a fresh ESP fetch by watching ageSec reset to a small value.
        // When the ESP completes a new API poll, ageSec drops back near 0.
        // We trigger an extra updateSpaceStatus() call a moment later so the
        // UI reflects the new data as soon as it arrives.
        var currentAge = (apiStatus && apiStatus.ageSec !== undefined) ? apiStatus.ageSec : -1;
        if (_lastAgeSec > 5 && currentAge >= 0 && currentAge < _lastAgeSec) {
            // ageSec reset — ESP just fetched new data; schedule an immediate refresh.
            setTimeout(updateSpaceStatus, 500);
        }
        _lastAgeSec = currentAge;

        // ESP has never completed a fetch yet (httpCode === 0) — keep the
        // hourglass indicator in place and try again shortly.
        if (apiStatus.httpCode === 0) {
            return;
        }

        var unmatchedSet = {};
        if (apiStatus.ok && apiStatus.parseErrors === 0 && apiStatus.unmatched) {
            apiStatus.unmatched.forEach(function(n) { unmatchedSet[n.toLowerCase().trim()] = true; });
        }

        if (list.length === 0) {
            document.querySelectorAll('#mapBody .space-status').forEach(function(s) {
                s.style.color = '#555'; s.title = 'No API data yet'; s.innerHTML = '&#8212;';
            });
            return;
        }

        var byLed  = {};
        var byName = {};
        list.forEach(function(e) {
            byLed[e.led]                        = e.status;
            byName[e.name.toLowerCase().trim()] = e.status;
        });
        document.querySelectorAll('#mapBody tr').forEach(function(row) {
            var ledHidden = row.querySelector('input[type=hidden]');
            var textInputs = row.querySelectorAll('input[type=text]');
            var span      = row.querySelector('.space-status');
            if (!ledHidden || !span) return;
            var led    = parseInt(ledHidden.value, 10);
            var nameInput = textInputs.length > 0 ? textInputs[0] : null;
            var name   = nameInput ? nameInput.value.toLowerCase().trim() : '';
            var status = (byLed[led] !== undefined) ? byLed[led] : byName[name];
            if (unmatchedSet[name]) {
                row.style.background = '#2a1a00'; row.style.outline = '1px solid #f5a800';
                span.style.color = '#f5a800'; span.title = 'Not found in last API response';
                span.innerHTML = '&#9888; N/A'; return;
            } else {
                if (row.style.background === 'rgb(42, 26, 0)') { row.style.background = ''; row.style.outline = ''; }
            }
            span.title = '';
            if      (status === 'OPEN')    { span.style.color = '#2dbe60'; span.innerHTML = '&#9679; OPEN'; }
            else if (status === 'CLOSED')  { span.style.color = '#e74c3c'; span.innerHTML = '&#9679; CLOSED'; }
            else if (status === 'UNKNOWN') { span.style.color = '#4a90d9'; span.innerHTML = '&#9679; UNKNOWN'; }
            else                           { span.style.color = '#555';    span.innerHTML = '&#8212;'; }
        });
    })
    .catch(function() {
        document.querySelectorAll('#mapBody .space-status').forEach(function(s) {
            s.style.color = '#e74c3c'; s.title = 'Status fetch failed'; s.innerHTML = '&#9888;';
        });
    });
}

// Initial call — zeigt Daten sofort wenn vorhanden, startet sonst den 2s-Retry.
updateSpaceStatus();
// Reguläres Polling alle 15 s als Fallback und für laufende Aktualisierung.
setInterval(updateSpaceStatus, 15000);

function reindexRows() {
    var rows = document.getElementById('mapBody').querySelectorAll('tr');
    rowCount = 0;
    rows.forEach(function(r) {
        r.id = 'row_' + rowCount;
        // Update hidden LED input name + value
        var ledHidden = r.querySelector('input[type=hidden]');
        if (ledHidden) { ledHidden.name = 'led_' + rowCount; ledHidden.value = rowCount; }
        // Update LED display label (text node inside 3rd td)
        var ledTd = r.querySelector('td:nth-child(3)');
        if (ledTd) {
            var tn = ledTd.lastChild;
            if (tn && tn.nodeType === 3) tn.nodeValue = rowCount;
        }
        // Update text input names
        var textInputs = r.querySelectorAll('input[type=text]');
        if (textInputs.length > 0) textInputs[0].name = 'name_' + rowCount;
        if (textInputs.length > 1) textInputs[1].name = 'city_' + rowCount;
        // Update disabled checkbox name
        var disCb = r.querySelector('.dis-check');
        if (disCb) disCb.name = 'dis_' + rowCount;
        rowCount++;
    });
}
</script>
)rawjs";

// JavaScript for / (index page).
static const char INDEX_JS[] PROGMEM = R"rawjs(
<script>
function checkApi() {
    fetch('/api/status')
        .then(function(r){ return r.json(); })
        .then(function(d) {
            var box = document.getElementById('apiStatus');
            var age = d.ageSec > 0 ? ' &mdash; last checked ' + d.ageSec + 's ago' : '';
            if (d.httpCode === 0) {
                box.style.borderColor = '#555';
                box.innerHTML = '<span style="font-size:1.1em">&#9711;</span>'
                    + ' <strong style="color:#aaa">No fetch yet</strong>'
                    + '<br><small style="color:#777">API URL: ' + d.url + '</small>';
            } else if (d.ok) {
                box.style.borderColor = '#1a7a3c';
                box.innerHTML = '<span style="font-size:1.1em">&#9679;</span>'
                    + ' <strong style="color:#2dbe60">SpaceMap API reachable</strong>' + age
                    + '<br><small style="color:#777">HTTP ' + d.httpCode + ' &mdash; ' + d.url + '</small>';
            } else {
                box.style.borderColor = '#c0392b';
                box.innerHTML = '<span style="font-size:1.1em">&#9679;</span>'
                    + ' <strong style="color:#e74c3c">SpaceMap API unreachable</strong>' + age
                    + '<br><small style="color:#777">HTTP ' + d.httpCode + ' &mdash; ' + d.url + '</small>';
            }
            var pbox = document.getElementById('parseStatus');
            if (d.httpCode === 0) { pbox.style.display = 'none'; return; }
            pbox.style.display = 'block';
            if (!d.ok) {
                pbox.style.borderColor = '#555';
                pbox.innerHTML = '<span style="color:#777;font-size:0.9em">&#8212; Parse status not available (HTTP error)</span>';
                return;
            }
            if (d.watchListSize === 0) {
                pbox.style.borderColor = '#555';
                pbox.innerHTML = '<span style="font-size:1.1em">&#9711;</span>'
                    + ' <strong style="color:#aaa">Watch list is empty &mdash; no matching needed</strong>';
                return;
            }
            if (d.parseErrors > 0) {
                pbox.style.borderColor = '#f5a800';
                pbox.innerHTML = '<span style="font-size:1.1em">&#9888;</span>'
                    + ' <strong style="color:#f5a800">Parse errors detected</strong>'
                    + '<br><small style="color:#aaa">'
                    + d.parseErrors + ' error(s) out of ' + d.totalObjects + ' objects &mdash; '
                    + d.foundCount + ' of ' + d.watchListSize + ' spaces matched</small>';
                return;
            }
            var notFound = d.unmatched ? d.unmatched.length : 0;
            if (notFound === 0) {
                pbox.style.borderColor = '#1a7a3c';
                pbox.innerHTML = '<span style="font-size:1.1em">&#10003;</span>'
                    + ' <strong style="color:#2dbe60">All ' + d.foundCount + ' of ' + d.watchListSize + ' spaces found in API</strong>';
            } else {
                pbox.style.borderColor = '#f5a800';
                pbox.innerHTML = '<span style="font-size:1.1em">&#9888;</span>'
                    + ' <strong style="color:#f5a800">' + d.foundCount + ' of ' + d.watchListSize + ' spaces found &mdash; '
                    + notFound + ' not in API response</strong>'
                    + '<br><small style="color:#aaa">Not found: ' + d.unmatched.join(', ') + '</small>';
            }
        })
        .catch(function() {
            document.getElementById('apiStatus').innerHTML =
                '<span style="color:#e74c3c">&#9888; Could not reach device</span>';
            document.getElementById('parseStatus').style.display = 'none';
        });
}
checkApi();
setInterval(checkApi, 30000);
</script>
)rawjs";

// --------------------------------------------------------------------------
// Public interface
// --------------------------------------------------------------------------
void SettingsWebServer::begin() {
    registerRoutes();
    _server.begin();
#ifdef DEBUG
    Serial.println("WEB: Settings server started on port 80");
#endif
}

// --------------------------------------------------------------------------
// Register all URL routes on the async web server.
// --------------------------------------------------------------------------
void SettingsWebServer::registerRoutes() {
    _httpsServer.on("/", HTTP_GET, [this](AsyncWebServerRequest* req) {
        handleHttpsRedirect(req);
    });
    _httpsServer.onNotFound([this](AsyncWebServerRequest* req) {
        handleHttpsRedirect(req);
    });
    _httpsServer.begin();

    _server.on("/", HTTP_GET, [this](AsyncWebServerRequest* req) {
        handleIndex(req);
    });

    _server.on("/settings", HTTP_GET, [this](AsyncWebServerRequest* req) {
        handleSettingsGet(req);
    });

    _server.on("/settings", HTTP_POST, [this](AsyncWebServerRequest* req) {
        handleSettingsPost(req);
    });

    _server.on("/settings/reset", HTTP_POST, [this](AsyncWebServerRequest* req) {
        handleSettingsReset(req);
    });

    // Sub-routes must be registered before the parent route, because
    // ESPAsyncWebServer matches handlers in registration order and
    // "/spacemap" would otherwise swallow "/spacemap/reset" etc.
    _server.on("/spacemap/reset", HTTP_POST, [this](AsyncWebServerRequest* req) {
        handleSpaceMapReset(req);
    });

    _server.on("/spacemap/export", HTTP_GET, [this](AsyncWebServerRequest* req) {
        handleSpaceMapExport(req);
    });

    _server.on("/spacemap/blink", HTTP_GET, [this](AsyncWebServerRequest* req) {
        handleSpaceMapBlink(req);
    });

    _server.on("/spacemap", HTTP_GET, [this](AsyncWebServerRequest* req) {
        handleSpaceMapGet(req);
    });

    _server.on("/spacemap", HTTP_POST, [this](AsyncWebServerRequest* req) {
        handleSpaceMapPost(req);
    });

    _server.on("/api/spacemap", HTTP_GET, [this](AsyncWebServerRequest* req) {
        handleApiSpaceMapGet(req);
    });

    _server.on("/api/spacestatus", HTTP_GET, [this](AsyncWebServerRequest* req) {
        AsyncResponseStream* s = req->beginResponseStream("application/json");
        s->print("[");
        for (size_t i = 0; i < spaceStatusList.size(); i++) {
            const auto& e = spaceStatusList[i];
            const char* statusStr;
            switch (e.getStatus()) {
                case SpaceStatus::OPEN:   statusStr = "OPEN";    break;
                case SpaceStatus::CLOSED: statusStr = "CLOSED";  break;
                default:                  statusStr = "UNKNOWN"; break;
            }
            if (i > 0) s->print(",");
            s->print("{\"name\":\"");
            s->print(e.getName());
            s->print("\",\"status\":\"");
            s->print(statusStr);
            s->print("\",\"led\":");
            s->print(e.getLED());
            s->print("}");
        }
        s->print("]");
        req->send(s);
    });

    _server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* req) {
        int code = WebClientHandler::getLastHttpCode();
        unsigned long age = (WebClientHandler::getLastAttemptMs() == 0)
                            ? 0
                            : (millis() - WebClientHandler::getLastAttemptMs()) / 1000UL;

        const auto& unmatched = WebClientHandler::getLastUnmatchedNames();
        String unmatchedJson = "[";
        for (size_t i = 0; i < unmatched.size(); i++) {
            if (i > 0) unmatchedJson += ",";
            unmatchedJson += "\"" + unmatched[i] + "\"";
        }
        unmatchedJson += "]";

        String json = "{\"httpCode\":"    + String(code) +
                      ",\"ok\":"          + (code == 200 ? "true" : "false") +
                      ",\"ageSec\":"      + String(age) +
                      ",\"url\":\""       + AppConfig::getInstance().getSpaceApiUrl() + "\"" +
                      ",\"foundCount\":"  + String(WebClientHandler::getLastFoundCount()) +
                      ",\"parseErrors\":" + String(WebClientHandler::getLastParseErrors()) +
                      ",\"totalObjects\":" + String(WebClientHandler::getLastTotalObjects()) +
                      ",\"watchListSize\":" + String(WebClientHandler::getLastWatchListSize()) +
                      ",\"unmatched\":"   + unmatchedJson +
                      "}";
        req->send(200, "application/json", json);
    });

    _server.onNotFound([this](AsyncWebServerRequest* req) {
        handleNotFound(req);
    });
}

// --------------------------------------------------------------------------
// Route handlers
// --------------------------------------------------------------------------
void SettingsWebServer::handleIndex(AsyncWebServerRequest* request) {
    AsyncResponseStream* s = request->beginResponseStream("text/html");
    streamIndexPage(s);
    request->send(s);
}

void SettingsWebServer::handleSettingsGet(AsyncWebServerRequest* request) {
    AsyncResponseStream* s = request->beginResponseStream("text/html");
    streamSettingsPage(s);
    request->send(s);
}

void SettingsWebServer::handleSettingsPost(AsyncWebServerRequest* request) {
    AppConfig& cfg = AppConfig::getInstance();

    auto getParam = [&](const String& key) -> String {
        if (!request->hasParam(key, true)) return "";
        return request->getParam(key, true)->value();
    };

    auto applyULong = [](const String& s, unsigned long minVal, unsigned long maxVal,
                         std::function<void(unsigned long)> setter) {
        if (s.length() == 0) return;
        long value = s.toInt();
        if (value > 0 && (unsigned long)value >= minVal && (unsigned long)value <= maxVal)
            setter((unsigned long)value);
    };

    auto applyUInt8 = [](const String& s, std::function<void(uint8_t)> setter) {
        if (s.length() == 0) return;
        int value = s.toInt();
        if (value >= 0 && value <= 255) setter((uint8_t)value);
    };

    auto applyUInt16 = [](const String& s, uint16_t minVal, uint16_t maxVal,
                          std::function<void(uint16_t)> setter) {
        if (s.length() == 0) return;
        int value = s.toInt();
        if (value >= minVal && value <= maxVal) setter((uint16_t)value);
    };

    String apiUrl = getParam("apiUrl");
    if (apiUrl.length() > 0) cfg.setSpaceApiUrl(apiUrl);

    applyULong(getParam("wifiInterval"),    30,   86400, [&](unsigned long v){ cfg.setIntervalWifiCheck(v); });
    applyULong(getParam("ledInterval"),      1,    3600, [&](unsigned long v){ cfg.setIntervalLEDs(v); });
    applyULong(getParam("apiInterval"),     10,    3600, [&](unsigned long v){ cfg.setIntervalApi(v); });
    applyUInt8(getParam("ledBrightness"),              [&](uint8_t v){ cfg.setLedBrightness(v); });
    applyUInt8(getParam("onboardBrightness"),          [&](uint8_t v){ cfg.setOnboardBrightness(v); });
    applyUInt16(getParam("ledMaxPower"), 100, 5000,    [&](uint16_t v){ cfg.setLedMaxPowerMa(v); });

    cfg.save();

#ifdef DEBUG
    Serial.println("WEB: Settings saved via web interface");
#endif

    AsyncResponseStream* s = request->beginResponseStream("text/html");
    streamSettingsPage(s, "Settings saved.");
    request->send(s);
}

void SettingsWebServer::handleSettingsReset(AsyncWebServerRequest* request) {
    AppConfig::getInstance().resetToDefaults();
#ifdef DEBUG
    Serial.println("WEB: Settings reset to defaults");
#endif
    AsyncResponseStream* s = request->beginResponseStream("text/html");
    streamSettingsPage(s, "Settings reset to defaults.");
    request->send(s);
}

void SettingsWebServer::handleNotFound(AsyncWebServerRequest* request) {
    AsyncResponseStream* s = request->beginResponseStream("text/html");
    s->print(htmlHeader("404"));
    s->print(navBar());
    s->print("<h2>404 - Page not found</h2><p><a href='/'>Back to home</a></p>");
    s->print(htmlFooter());
    request->send(s);
}

void SettingsWebServer::handleHttpsRedirect(AsyncWebServerRequest* request) {
    String ip  = WiFi.localIP().toString();
    String url = request->url();
    String html = "<!DOCTYPE html><html><head>"
                  "<meta http-equiv='refresh' content='0;url=http://" + ip + url + "'>"
                  "</head><body>"
                  "<p>Redirecting to <a href='http://" + ip + url + "'>http://" + ip + url + "</a></p>"
                  "</body></html>";
    request->send(200, "text/html", html);
}

// --------------------------------------------------------------------------
// HTML fragment helpers
// --------------------------------------------------------------------------
String SettingsWebServer::htmlHeader(const String& title) {
    String h;
    h.reserve(512);
    h  = "<!DOCTYPE html><html lang='en'><head>"
         "<meta charset='UTF-8'>"
         "<meta name='viewport' content='width=device-width, initial-scale=1'>"
         "<title>SpaceMap - ";
    h += title;
    h += "</title><style>";
    h += FPSTR(CSS);   // read from PROGMEM, no DRAM copy retained
    h += "</style></head><body>";
    return h;
}

String SettingsWebServer::htmlFooter() {
    return "</div></body></html>";
}

String SettingsWebServer::navBar() {
    return "<nav>"
           "<a href='/'>&#127968; Overview</a>"
           "<a href='/settings'>&#9881; Settings</a>"
           "<a href='/spacemap'>&#128280; SpaceMap</a>"
           "</nav><div class='container'>";
}

// --------------------------------------------------------------------------
// Streaming page renderers
// Each function writes into an AsyncResponseStream in small pieces so that
// the ESP32 heap never needs to hold the full page as one String object.
// --------------------------------------------------------------------------

void SettingsWebServer::streamIndexPage(AsyncResponseStream* s, const String& message) {
    s->print(htmlHeader("Overview"));
    s->print(navBar());
    s->print("<h1>SpaceMap on Fiber</h1>");
    s->print("<p>Welcome to the SpaceMap controller web interface.</p>");
    s->print("<div id='apiStatus' style='margin-top:20px;padding:14px 18px;border-radius:6px;"
             "background:#1a1a1a;border:1px solid #3a3a3a;max-width:480px;'>"
             "<span style='color:#888;font-size:0.9em'>&#8635; Checking API connection&hellip;</span>"
             "</div>");
    s->print("<div id='parseStatus' style='margin-top:10px;padding:14px 18px;border-radius:6px;"
             "background:#1a1a1a;border:1px solid #3a3a3a;max-width:480px;display:none;'></div>");
    s->print(FPSTR(INDEX_JS));
    if (message.length() > 0) {
        s->print("<div class='msg'>");
        s->print(message);
        s->print("</div>");
    }
    s->print(htmlFooter());
}

void SettingsWebServer::streamSettingsPage(AsyncResponseStream* s, const String& message) {
    AppConfig& cfg = AppConfig::getInstance();

    s->print(htmlHeader("Settings"));
    s->print(navBar());
    s->print("<h1>Settings</h1>");

    if (message.length() > 0) {
        bool isReset = message.indexOf("reset") >= 0;
        s->print("<div class='msg");
        if (isReset) s->print(" msg-reset");
        s->print("'>");
        s->print(message);
        s->print("</div>");
    }

    s->print("<form method='POST' action='/settings'>");
    s->print("<h2>API</h2>");
    s->print("<label>SpaceAPI URL</label>"
             "<input type='url' name='apiUrl' value='");
    s->print(cfg.getSpaceApiUrl());
    s->print("'>");

    s->print("<label>API polling interval (seconds)</label>"
             "<input type='number' name='apiInterval' min='10' max='3600' value='");
    s->print(cfg.getIntervalApi());
    s->print("'>");

    s->print("<h2>WiFi</h2>");
    s->print("<label>WiFi health-check interval (seconds)</label>"
             "<input type='number' name='wifiInterval' min='30' max='86400' value='");
    s->print(cfg.getIntervalWifiCheck());
    s->print("'>");

    s->print("<h2>LEDs</h2>");
    s->print("<label>LED refresh interval (seconds)</label>"
             "<input type='number' name='ledInterval' min='1' max='3600' value='");
    s->print(cfg.getIntervalLEDs());
    s->print("'>");

    s->print("<label>LED brightness (0-255)</label>"
             "<input type='number' name='ledBrightness' min='0' max='255' value='");
    s->print(cfg.getLedBrightness());
    s->print("'>");

    s->print("<label>Onboard LED brightness (0-255)</label>"
             "<input type='number' name='onboardBrightness' min='0' max='255' value='");
    s->print(cfg.getOnboardBrightness());
    s->print("'>");

    s->print("<label>Max. LED power draw (mA)</label>"
             "<input type='number' name='ledMaxPower' min='100' max='5000' value='");
    s->print(cfg.getLedMaxPowerMa());
    s->print("'>");

    s->print("<br><button type='submit' class='btn btn-save'>&#128190; Save</button>");
    s->print("</form>");

    s->print("<form method='POST' action='/settings/reset' "
             "onsubmit=\"return confirm('Reset all settings to defaults?')\">"
             "<button type='submit' class='btn btn-reset'>&#8635; Reset to defaults</button>"
             "</form>");

    s->print(htmlFooter());
}

void SettingsWebServer::streamSpaceMapPage(AsyncResponseStream* s, const String& message) {
    s->print(htmlHeader("SpaceMap"));
    s->print(navBar());
    s->print("<h1>Hackerspace Mapping</h1>");

    // --- Editor table ---
    s->print("<h2>Active Mapping</h2>"
             "<form method='POST' action='/spacemap' id='mapForm'>"
             "<table id='mapTable' style='width:100%;border-collapse:collapse;margin-top:8px'>"
             "<thead><tr>"
             "<th style='padding:6px;border-bottom:1px solid #3a3a3a;width:24px' title='Zeile ziehen zum Umsortieren'></th>"
             "<th style='padding:6px;border-bottom:1px solid #3a3a3a;text-align:center'>"
               "<span style='color:#888;font-size:0.8em'>Aus &middot; Blinken &middot; Status</span>"
               "<br><small style='color:#555;font-size:0.7em'>Deaktiviert &middot; LED orten &middot; API-Status</small>"
             "</th>"
             "<th style='text-align:center;padding:6px;color:#1a7a3c;border-bottom:1px solid #3a3a3a;width:40px' title='LED-Index (0 = erste LED)'>LED#</th>"
             "<th style='text-align:left;padding:6px;color:#1a7a3c;border-bottom:1px solid #3a3a3a' title='Name exakt wie in der SpaceAPI'>"
               "Space Name<br><small style='color:#555;font-weight:normal;font-size:0.75em'>Name exakt wie in SpaceAPI</small>"
             "</th>"
             "<th style='text-align:left;padding:6px;color:#1a7a3c;border-bottom:1px solid #3a3a3a' title='Stadt (optional, nur zur Anzeige)'>"
               "City<br><small style='color:#555;font-weight:normal;font-size:0.75em'>optional, zur Anzeige</small>"
             "</th>"
             "</tr></thead>"
             "<tbody id='mapBody'>");

    // tbody is populated by JS via fetch('/api/spacemap') — no rows rendered here.
    // This keeps the initial HTML response small and avoids heap pressure on the ESP32.

    s->print("</tbody></table>"
             "<div style='margin-top:12px;display:flex;align-items:center;flex-wrap:wrap;gap:10px'>"
             "<button type='submit' class='btn btn-save' style='margin:0' id='saveBtn'>&#128190; Save</button>");

    if (message.length() > 0) {
        bool isReset = message.indexOf("eset") >= 0;
        bool isError = message.indexOf("rror") >= 0;
        const char* color = isError ? "#f88" : "#6fcf6f";
        const char* bg    = isError ? "#2e0e0e" : (isReset ? "#2e1a00" : "#0e2e0e");
        s->print("<span id='saveMsg' style='padding:6px 14px;border-radius:4px;font-weight:bold;background:");
        s->print(bg); s->print(";color:"); s->print(color); s->print("'>");
        s->print(message);
        s->print("</span>");
    } else {
        s->print("<span id='saveMsg' style='display:none'></span>");
    }
    s->print("</div></form>");

    // --- Import ---
    s->print("<h2>Import</h2>"
             "<p style='color:#bbb;font-size:0.9em'>Eintr&auml;ge einfügen und Import klicken. Erwartetes Format:</p>"
             "<pre style='background:#1a1a1a;border:1px solid #2a2a2a;border-radius:4px;padding:10px;"
             "font-size:0.8em;color:#888;overflow-x:auto;margin:0 0 10px 0;'>"
             "{ 2, \"MuCCC\", \"Munich\"}&#10;"
             "{ 2, \"MuCCC\", \"Munich\", 1}  &larr; deaktiviert&#10;"
             "{ \"MuCCC\", \"Munich\"}"
             "</pre>"
             "<textarea id='importArea' rows='6' style='width:100%;background:#1e1e1e;color:#eee;"
             "border:1px solid #3a3a3a;border-radius:4px;padding:7px;font-family:monospace;"
             "font-size:0.85em;box-sizing:border-box;'></textarea>"
             "<div style='margin-top:8px;display:flex;gap:10px;align-items:center;flex-wrap:wrap'>"
             "<button type='button' class='btn btn-save' style='margin:0' onclick='doImport()'>&#8659; Import</button>"
             "</div>");

    // --- Export ---
    s->print("<h2>Export</h2>"
             "<p style='color:#bbb;font-size:0.9em'>L&auml;dt das aktuelle Mapping als <code>searchList.txt</code> herunter "
             "(alle Eintr&auml;ge inkl. leerer Positionen).</p>"
             "<button type='button' class='btn btn-save' onclick='doExport()'>&#8659; Export searchList.txt</button>");

    // --- Reset ---
    s->print("<h2>Reset</h2>"
             "<form method='POST' action='/spacemap/reset' "
             "onsubmit=\"return confirm('Reset mapping to built-in default?')\">"
             "<button type='submit' class='btn btn-reset'>&#8635; Reset to default</button>"
             "</form>");

    // Inject LED_COUNT so JS knows how many rows to expect, then load the mapping
    // asynchronously from /api/spacemap to keep the initial HTML small.
    s->print("<script>var LED_COUNT = ");
    s->print(LED_COUNT);
    s->print("; var rowCount = 0;</script>"
             "<script>"
             "fetch('/api/spacemap').then(function(r){return r.json();}).then(function(entries){"
             "var tbody=document.getElementById('mapBody');"
             "var html='';"
             "for(var i=0;i<LED_COUNT;i++){"
             "var e=entries[i]||{led:i,name:'',city:'',disabled:false};"
             "html+=buildRow(i,e.name,e.city,e.disabled);}"
             "tbody.innerHTML=html;"
             "rowCount=LED_COUNT;"
             "updateSpaceStatus();"
             "}).catch(function(){document.getElementById('mapBody').innerHTML="
             "'<tr><td colspan=5 style=color:#e74c3c>&#9888; Failed to load mapping from /api/spacemap</td></tr>';});"
             "</script>");
    s->print(FPSTR(SPACEMAP_JS));

    s->print(htmlFooter());
}

// --------------------------------------------------------------------------
// /spacemap  – GET
// --------------------------------------------------------------------------
void SettingsWebServer::handleSpaceMapGet(AsyncWebServerRequest* request) {
    String msg = "";
    if (request->hasParam("msg")) {
        String key = request->getParam("msg")->value();
        if      (key == "saved") msg = "Mapping saved.";
        else if (key == "reset") msg = "Mapping reset to default.";
    }
    AsyncResponseStream* s = request->beginResponseStream("text/html");
    streamSpaceMapPage(s, msg);
    request->send(s);
}

// --------------------------------------------------------------------------
// /spacemap  – POST: save edited list
// --------------------------------------------------------------------------
void SettingsWebServer::handleSpaceMapPost(AsyncWebServerRequest* request) {
    std::vector<SpaceSearchList> newList;
    // Always iterate exactly LED_COUNT slots — empty entries are stored as empty strings.
    for (int idx = 0; idx < LED_COUNT; idx++) {
        String ledKey  = "led_"  + String(idx);
        String nameKey = "name_" + String(idx);
        String cityKey = "city_" + String(idx);
        String disKey  = "dis_"  + String(idx);

        if (!request->hasParam(ledKey, true)) break;

        String ledStr  = request->getParam(ledKey,  true)->value();
        String nameStr = request->hasParam(nameKey, true) ? request->getParam(nameKey, true)->value() : "";
        String cityStr = request->hasParam(cityKey, true) ? request->getParam(cityKey, true)->value() : "";
        // Checkboxes are only submitted when checked — absence means unchecked.
        bool   disVal  = request->hasParam(disKey, true);

        uint8_t ledNum = (uint8_t)constrain(ledStr.toInt(), 0, 255);
        // Store every slot — empty name means LED will be off (black).
        newList.emplace_back(ledNum, nameStr, cityStr, disVal);
    }

    DataSpaceList::getInstance().saveList(newList);

#ifdef DEBUG
    Serial.println("WEB: SpaceMap saved via web interface");
#endif
    // 303 redirect — avoids re-rendering the full page in the POST handler,
    // which would overflow the async task stack with LED_COUNT entries.
    AsyncWebServerResponse* resp = request->beginResponse(303);
    resp->addHeader("Location", "/spacemap?msg=saved");
    request->send(resp);
}

// --------------------------------------------------------------------------
// /spacemap/reset  – POST
// --------------------------------------------------------------------------
void SettingsWebServer::handleSpaceMapReset(AsyncWebServerRequest* request) {
    DataSpaceList::getInstance().resetToDefault();
#ifdef DEBUG
    Serial.println("WEB: SpaceMap reset to default");
#endif
    AsyncWebServerResponse* resp = request->beginResponse(303);
    resp->addHeader("Location", "/spacemap?msg=reset");
    request->send(resp);
}

// --------------------------------------------------------------------------
// /spacemap/blink  – GET
// --------------------------------------------------------------------------
void SettingsWebServer::handleSpaceMapBlink(AsyncWebServerRequest* request) {
    if (!request->hasParam("led")) {
        request->send(400, "application/json", "{\"ok\":false,\"error\":\"missing led param\"}");
        return;
    }
    int ledIndex = request->getParam("led")->value().toInt();
    if (ledIndex < 0 || ledIndex >= LED_COUNT) {
        request->send(400, "application/json", "{\"ok\":false,\"error\":\"led index out of range\"}");
        return;
    }
    request->send(200, "application/json", "{\"ok\":true}");

    struct BlinkParams {
        uint8_t ledIndex;
        std::vector<SpaceStatusList> snapshot;
    };
    auto* p = new BlinkParams{(uint8_t)ledIndex, spaceStatusList};

    xTaskCreate([](void* arg) {
        auto* bp = static_cast<BlinkParams*>(arg);
        NeoPixelLED::getInstance().blinkLED(bp->ledIndex, bp->snapshot);
        delete bp;
        vTaskDelete(nullptr);
    }, "blink_task", 4096, p, 1, nullptr);
}

// --------------------------------------------------------------------------
// /api/spacemap  – GET: compact JSON for the editor table
// --------------------------------------------------------------------------
void SettingsWebServer::handleApiSpaceMapGet(AsyncWebServerRequest* request) {
    const auto& list = DataSpaceList::getInstance().getList();

    std::vector<const SpaceSearchList*> byLed(LED_COUNT, nullptr);
    for (const auto& e : list) {
        if (e.getLED() < LED_COUNT) byLed[e.getLED()] = &e;
    }

    AsyncResponseStream* s = request->beginResponseStream("application/json");
    s->print("[");
    for (int i = 0; i < LED_COUNT; i++) {
        if (i > 0) s->print(",");
        const SpaceSearchList* e = byLed[i];
        s->print("{\"led\":");
        s->print(i);
        s->print(",\"name\":\"");
        s->print(e ? e->getName() : "");
        s->print("\",\"city\":\"");
        s->print(e ? e->city : "");
        s->print("\",\"disabled\":");
        s->print((e && e->isDisabled()) ? "true" : "false");
        s->print("}");
    }
    s->print("]");
    request->send(s);
}

// --------------------------------------------------------------------------
// /spacemap/export  – GET
// --------------------------------------------------------------------------
void SettingsWebServer::handleSpaceMapExport(AsyncWebServerRequest* request) {
    const auto& list = DataSpaceList::getInstance().getList();

    // Build a lookup by LED index so we can iterate all LED_COUNT slots in order.
    std::vector<SpaceSearchList const*> byLed(LED_COUNT, nullptr);
    for (const auto& e : list) {
        if (e.getLED() < LED_COUNT) byLed[e.getLED()] = &e;
    }

    AsyncResponseStream* s = request->beginResponseStream("application/octet-stream");
    s->addHeader("Content-Disposition", "attachment; filename=\"searchList.txt\"");
    s->addHeader("Content-Transfer-Encoding", "binary");
    for (int i = 0; i < LED_COUNT; i++) {
        if (i > 0) s->print(", ");
        if (byLed[i]) {
            s->print("{ ");
            s->print(i);
            s->print(", \"");
            s->print(byLed[i]->getName());
            s->print("\", \"");
            s->print(byLed[i]->city);
            s->print("\"}");
        } else {
            s->print("{ ");
            s->print(i);
            s->print(", \"\", \"\"}");
        }
    }
    s->print("\n");
    request->send(s);
}
