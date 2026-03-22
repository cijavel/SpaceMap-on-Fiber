#include "SettingsWebServer.h"
#include "AppConfig.h"
#include "Configuration.h"
#include "DataSpaceList.h"
#include "WebClientHandler.h"
#include <ArduinoJson.h>

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

    _server.on("/spacemap", HTTP_GET, [this](AsyncWebServerRequest* req) {
        handleSpaceMapGet(req);
    });

    _server.on("/spacemap", HTTP_POST, [this](AsyncWebServerRequest* req) {
        handleSpaceMapPost(req);
    });
    _server.on("/spacemap/reset", HTTP_POST, [this](AsyncWebServerRequest* req) {
        handleSpaceMapReset(req);
    });

    _server.on("/spacemap/export", HTTP_GET, [this](AsyncWebServerRequest* req) {
        handleSpaceMapExport(req);
    });

    _server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* req) {
        int code = WebClientHandler::getLastHttpCode();
        unsigned long age = (WebClientHandler::getLastAttemptMs() == 0)
                            ? 0
                            : (millis() - WebClientHandler::getLastAttemptMs()) / 1000UL;
        String json = "{\"httpCode\":" + String(code) +
                      ",\"ok\":" + (code == 200 ? "true" : "false") +
                      ",\"ageSec\":" + String(age) +
                      ",\"url\":\"" + AppConfig::getInstance().getSpaceApiUrl() + "\"}";
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
    request->send(200, "text/html", buildIndexPage());
}

void SettingsWebServer::handleSettingsGet(AsyncWebServerRequest* request) {
    request->send(200, "text/html", buildSettingsPage());
}

void SettingsWebServer::handleSettingsPost(AsyncWebServerRequest* request) {
    AppConfig& cfg = AppConfig::getInstance();

    // Read POST parameters directly – ESPAsyncWebServer parses URL-encoded bodies automatically.
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

    request->send(200, "text/html", buildSettingsPage("Settings saved."));
}

void SettingsWebServer::handleSettingsReset(AsyncWebServerRequest* request) {
    AppConfig::getInstance().resetToDefaults();
#ifdef DEBUG
    Serial.println("WEB: Settings reset to defaults");
#endif
    request->send(200, "text/html", buildSettingsPage("Settings reset to defaults."));
}

void SettingsWebServer::handleNotFound(AsyncWebServerRequest* request) {
    request->send(404, "text/html",
        htmlHeader("404") +
        "<h2>404 - Page not found</h2>"
        "<p><a href='/'>Back to home</a></p>" +
        htmlFooter());
}

void SettingsWebServer::handleHttpsRedirect(AsyncWebServerRequest* request) {
    String html = "<!DOCTYPE html><html><head>"
                  "<meta http-equiv='refresh' content='0;url=http://" +
                  WiFi.localIP().toString() + request->url() + "'>"
                  "</head><body>"
                  "<p>Redirecting to <a href='http://" +
                  WiFi.localIP().toString() + request->url() + "'>http://" +
                  WiFi.localIP().toString() + request->url() + "</a></p>"
                  "</body></html>";
    request->send(200, "text/html", html);
}

// --------------------------------------------------------------------------
// HTML helpers
// --------------------------------------------------------------------------
String SettingsWebServer::htmlHeader(const String& title) {
    return String(
        "<!DOCTYPE html><html lang='en'><head>"
        "<meta charset='UTF-8'>"
        "<meta name='viewport' content='width=device-width, initial-scale=1'>"
        "<title>SpaceMap - ") + title + "</title>"
        "<style>"
        "body{font-family:sans-serif;margin:0;background:#111;color:#eee;}"
        "nav{background:#1a1a1a;padding:10px 20px;border-bottom:2px solid #e02020;}"
        "nav a{color:#1a7a3c;text-decoration:none;margin-right:16px;font-weight:bold;}"
        "nav a:hover{color:#fff;text-decoration:underline;}"
        ".container{max-width:700px;margin:30px auto;padding:0 16px;}"
        "h1{color:#1a7a3c;}"
        "h2{color:#f5a800;}"
        "label{display:block;margin-top:14px;font-size:0.9em;color:#bbb;}"
        "input[type=text],input[type=number],input[type=url]"
        "{width:100%;padding:7px;background:#1e1e1e;color:#eee;border:1px solid #3a3a3a;"
        "border-radius:4px;box-sizing:border-box;margin-top:4px;}"
        "input:focus{outline:2px solid #1a7a3c;border-color:#1a7a3c;}"
        ".btn{display:inline-block;margin-top:20px;padding:10px 24px;"
        "border:none;border-radius:5px;cursor:pointer;font-size:1em;font-weight:bold;}"
        ".btn-save{background:#1a7a3c;color:#fff;}"
        ".btn-save:hover{background:#2d9e55;}"
        ".btn-reset{background:#f5a800;color:#fff;margin-left:12px;}"
        ".btn-reset:hover{background:#ffc030;}"
        ".btn-settings{display:inline-block;margin-top:20px;padding:10px 24px;"
        "background:#1a7a3c;color:#fff;border:none;border-radius:5px;cursor:pointer;"
        "font-size:1em;font-weight:bold;text-decoration:none;}"
        ".btn-settings:hover{background:#24a050;}"
        ".msg{margin-top:14px;padding:10px;background:#0e2e0e;border-left:4px solid #1a7a3c;"
        "border-radius:4px;color:#6fcf6f;}"
        ".msg-reset{background:#2e0e0e;border-left:4px solid #f5a800;color:#f88;}"
        "</style></head><body>";
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

String SettingsWebServer::buildIndexPage(const String& message) {
    String html = htmlHeader("Overview") + navBar();
    html += "<h1>SpaceMap on Fiber</h1>";
    html += "<p>Welcome to the SpaceMap controller web interface.</p>";
    html += "<div id='apiStatus' style='margin-top:20px;padding:14px 18px;border-radius:6px;"
            "background:#1a1a1a;border:1px solid #3a3a3a;max-width:480px;'>"
            "<span style='color:#888;font-size:0.9em'>&#8635; Checking API connection&hellip;</span>"
            "</div>";
    html += R"rawjs(
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
                        + ' <strong style="color:#2dbe60">SpaceMap API reachable</strong>'
                        + age
                        + '<br><small style="color:#777">HTTP ' + d.httpCode + ' &mdash; ' + d.url + '</small>';
                } else {
                    box.style.borderColor = '#c0392b';
                    box.innerHTML = '<span style="font-size:1.1em">&#9679;</span>'
                        + ' <strong style="color:#e74c3c">SpaceMap API unreachable</strong>'
                        + age
                        + '<br><small style="color:#777">HTTP ' + d.httpCode + ' &mdash; ' + d.url + '</small>';
                }
            })
            .catch(function() {
                document.getElementById('apiStatus').innerHTML =
                    '<span style="color:#e74c3c">&#9888; Could not reach device</span>';
            });
    }
    checkApi();
    setInterval(checkApi, 30000);
    </script>
    )rawjs";
    if (message.length() > 0) {
        html += "<div class='msg'>" + message + "</div>";
    }
    return html + htmlFooter();
}

String SettingsWebServer::buildSettingsPage(const String& message) {
    AppConfig& cfg = AppConfig::getInstance();
    String html = htmlHeader("Settings") + navBar();
    html += "<h1>Settings</h1>";

    if (message.length() > 0) {
        bool isReset = message.indexOf("reset") >= 0;
        html += "<div class='msg" + String(isReset ? " msg-reset" : "") + "'>" + message + "</div>";
    }

    html += "<form method='POST' action='/settings'>";

    html += "<h2>API</h2>";
    html += "<label>SpaceAPI URL</label>"
            "<input type='url' name='apiUrl' value='" + cfg.getSpaceApiUrl() + "'>";
    html += "<label>API polling interval (seconds)</label>"
            "<input type='number' name='apiInterval' min='10' max='3600' value='" +
            String(cfg.getIntervalApi()) + "'>";

    html += "<h2>WiFi</h2>";
    html += "<label>WiFi health-check interval (seconds)</label>"
            "<input type='number' name='wifiInterval' min='30' max='86400' value='" +
            String(cfg.getIntervalWifiCheck()) + "'>";

    html += "<h2>LEDs</h2>";
    html += "<label>LED refresh interval (seconds)</label>"
            "<input type='number' name='ledInterval' min='1' max='3600' value='" +
            String(cfg.getIntervalLEDs()) + "'>";
    html += "<label>LED brightness (0-255)</label>"
            "<input type='number' name='ledBrightness' min='0' max='255' value='" +
            String(cfg.getLedBrightness()) + "'>";
    html += "<label>Onboard LED brightness (0-255)</label>"
            "<input type='number' name='onboardBrightness' min='0' max='255' value='" +
            String(cfg.getOnboardBrightness()) + "'>";
    html += "<label>Max. LED power draw (mA)</label>"
            "<input type='number' name='ledMaxPower' min='100' max='5000' value='" +
            String(cfg.getLedMaxPowerMa()) + "'>";

    html += "<br><button type='submit' class='btn btn-save'>&#128190; Save</button>";
    html += "</form>";

    html += "<form method='POST' action='/settings/reset' "
            "onsubmit=\"return confirm('Reset all settings to defaults?')\">"
            "<button type='submit' class='btn btn-reset'>&#8635; Reset to defaults</button>"
            "</form>";

    return html + htmlFooter();
}


// --------------------------------------------------------------------------
// /spacemap  – GET: show editor
// --------------------------------------------------------------------------
void SettingsWebServer::handleSpaceMapGet(AsyncWebServerRequest* request) {
    request->send(200, "text/html", buildSpaceMapPage());
}

// --------------------------------------------------------------------------
// /spacemap  – POST: save edited list
// --------------------------------------------------------------------------
void SettingsWebServer::handleSpaceMapPost(AsyncWebServerRequest* request) {
    // ESPAsyncWebServer parses URL-encoded bodies automatically – use getParam() directly.
    std::vector<SpaceSearchList> newList;
    int idx = 0;
    while (idx < SPACEMAP_MAX_ENTRIES) {
        String ledKey  = "led_"  + String(idx);
        String nameKey = "name_" + String(idx);
        String cityKey = "city_" + String(idx);

        if (!request->hasParam(ledKey, true)) break;  // No more entries.

        String ledStr  = request->getParam(ledKey,  true)->value();
        String nameStr = request->hasParam(nameKey, true) ? request->getParam(nameKey, true)->value() : "";
        String cityStr = request->hasParam(cityKey, true) ? request->getParam(cityKey, true)->value() : "";

        if (nameStr.length() > 0) {
            uint8_t ledNum = (uint8_t)constrain(ledStr.toInt(), 0, 255);
            newList.emplace_back(ledNum, nameStr, cityStr);
        }
        idx++;
    }

    DataSpaceList::getInstance().saveList(newList);

#ifdef DEBUG
    Serial.println("WEB: SpaceMap saved via web interface");
#endif
    request->send(200, "text/html", buildSpaceMapPage("Mapping saved."));
}

// --------------------------------------------------------------------------
// /spacemap/reset  – POST: restore built-in default
// --------------------------------------------------------------------------
void SettingsWebServer::handleSpaceMapReset(AsyncWebServerRequest* request) {
    DataSpaceList::getInstance().resetToDefault();
#ifdef DEBUG
    Serial.println("WEB: SpaceMap reset to default");
#endif
    request->send(200, "text/html", buildSpaceMapPage("Mapping reset to default."));
}

// --------------------------------------------------------------------------
// /spacemap/export  – GET: deliver a .cpp snippet as plain text download
// --------------------------------------------------------------------------
void SettingsWebServer::handleSpaceMapExport(AsyncWebServerRequest* request) {
    const auto& list = DataSpaceList::getInstance().getList();
    String out = "";
    for (int i = 0; i < (int)list.size(); i++) {
        out += "{ " + String(list[i].getLED()) +
               ", \"" + list[i].getName() +
               "\", \"" + list[i].city + "\"}";
        if (i < (int)list.size() - 1) out += ", ";
    }
    out += "\n";

    AsyncWebServerResponse* resp = request->beginResponse(200, "application/octet-stream", out);
    resp->addHeader("Content-Disposition", "attachment; filename=\"searchList.txt\"");
    resp->addHeader("Content-Transfer-Encoding", "binary");
    request->send(resp);
}

// --------------------------------------------------------------------------
// Build the /spacemap HTML page
// --------------------------------------------------------------------------
String SettingsWebServer::buildSpaceMapPage(const String& message) {
    const auto& list = DataSpaceList::getInstance().getList();
    String html = htmlHeader("SpaceMap") + navBar();
    html += "<h1>LED &#8596; Hackerspace Mapping</h1>";

    // --- Import area ---
    html += "<h2>Import</h2>"
            "<p style='color:#bbb;font-size:0.9em'>Paste your mapping below and click Import. Expected format:</p>"
            "<pre style='background:#1a1a1a;border:1px solid #2a2a2a;border-radius:4px;padding:10px;"
            "font-size:0.8em;color:#888;overflow-x:auto;margin:0 0 10px 0;'>"
            "{ 0, \"OpenLab Augsburg\", \"Augsburg\"}, { 1, \"IT-Syndikat\", \"Innsbruck\"}, { 2, \"MuCCC\", \"Munich\"}"
            "</pre>"
            "<textarea id='importArea' rows='8' style='width:100%;background:#1e1e1e;color:#eee;"
            "border:1px solid #3a3a3a;border-radius:4px;padding:7px;font-family:monospace;font-size:0.85em;box-sizing:border-box;'>"
            "</textarea>"
            "<div style='margin-top:8px;display:flex;gap:10px;align-items:center;flex-wrap:wrap'>"
            "<button type='button' class='btn btn-save' style='margin:0' onclick='doImport(false)'>&#8659; Import (ersetzen)</button>"
            "<button type='button' class='btn btn-save' style='margin:0;background:#2a5a2a' onclick='doImport(true)'>&#43; Import (anh&auml;ngen)</button>"
            "</div>";

    // --- Editor table ---
    html += "<h2>Active Mapping</h2>"
            "<style>input.led-input::-webkit-outer-spin-button,input.led-input::-webkit-inner-spin-button{-webkit-appearance:none;margin:0;}</style>"
            "<form method='POST' action='/spacemap' id='mapForm'>"
            "<table id='mapTable' style='width:100%;border-collapse:collapse;margin-top:8px'>"
            "<thead><tr>"
            "<th style='padding:6px;border-bottom:1px solid #3a3a3a;width:24px'></th>"
            "<th style='text-align:left;padding:6px;color:#1a7a3c;border-bottom:1px solid #3a3a3a'>LED#</th>"
            "<th style='text-align:left;padding:6px;color:#1a7a3c;border-bottom:1px solid #3a3a3a'>Space Name</th>"
            "<th style='text-align:left;padding:6px;color:#1a7a3c;border-bottom:1px solid #3a3a3a'>City</th>"
            "<th style='padding:6px;border-bottom:1px solid #3a3a3a'></th>"
            "</tr></thead>"
            "<tbody id='mapBody'>";

    for (int i = 0; i < (int)list.size(); i++) {
        html += buildSpaceMapRow(i, list[i].getLED(), list[i].getName(), list[i].city);
    }

    html += "</tbody></table>"
            "<div style='margin-top:12px;display:flex;align-items:center;flex-wrap:wrap;gap:10px'>"
            "<button type='button' class='btn btn-save' style='background:#1a7a3c;margin:0' onclick='addRow()'>&#43; Add row</button>"
            "<button type='submit' class='btn btn-save' style='margin:0' id='saveBtn'>&#128190; Save</button>";
    if (message.length() > 0) {
        bool isReset = message.indexOf("eset") >= 0;
        bool isError = message.indexOf("rror") >= 0;
        String color = isError ? "#f88" : "#6fcf6f";
        String bg    = isError ? "#2e0e0e" : (isReset ? "#2e1a00" : "#0e2e0e");
        html += "<span id='saveMsg' style='padding:6px 14px;border-radius:4px;font-weight:bold;"
                "background:" + bg + ";color:" + color + "'>" + message + "</span>";
    } else {
        html += "<span id='saveMsg' style='display:none'></span>";
    }
    html += "</div></form>";

    // --- Export ---
    html += "<h2>Export</h2>"
            "<p style='color:#bbb;font-size:0.9em'>Downloads the current mapping as a <code>searchList.cpp</code> snippet "
            "ready to paste into <code>DataSpaceList.cpp</code>.</p>"
            "<button type='button' class='btn btn-save' onclick='doExport()'>&#8659; Export searchList.txt</button>";

    // --- Reset ---
    html += "<h2>Reset</h2>"
            "<form method='POST' action='/spacemap/reset' "
            "onsubmit=\"return confirm('Reset mapping to built-in default?')\">"
            "<button type='submit' class='btn btn-reset'>&#8635; Reset to default</button>"
            "</form>";

    // --- JavaScript ---
    html += R"rawjs(
<script>
var rowCount = )rawjs" + String(list.size()) + R"rawjs(;

function buildRow(i, led, name, city) {
    return '<tr id="row_'+i+'" draggable="true" ondragstart="onDragStart(event)" ondragover="onDragOver(event)" ondrop="onDrop(event)" ondragend="onDragEnd(event)" style="cursor:grab">'
        + '<td style="padding:4px;width:24px;color:#555;font-size:1.2em;text-align:center;cursor:grab" title="Drag to reorder">&#8597;</td>'
        + '<td style="padding:4px"><input type="number" name="led_'+i+'" value="'+led+'" min="0" max="255" style="width:60px;background:#1e1e1e;color:#eee;border:1px solid #3a3a3a;border-radius:4px;padding:4px;-moz-appearance:textfield" class="led-input" onchange="onLedChanged(this)"></td>'
        + '<td style="padding:4px"><input type="text"   name="name_'+i+'" value="'+name+'" style="width:100%;background:#1e1e1e;color:#eee;border:1px solid #3a3a3a;border-radius:4px;padding:4px" oninput="markRowDirty(this.closest(\'tr\'))"></td>'
        + '<td style="padding:4px"><input type="text"   name="city_'+i+'" value="'+city+'" style="width:100%;background:#1e1e1e;color:#eee;border:1px solid #3a3a3a;border-radius:4px;padding:4px" oninput="markRowDirty(this.closest(\'tr\'))"></td>'
        + '<td style="padding:4px"><button type="button" onclick="removeRow('+i+')" style="background:#f5a800;color:#fff;border:none;border-radius:4px;padding:4px 10px;cursor:pointer">&#10005;</button></td>'
        + '</tr>';
}

function markRowDirty(row) {
    row.style.background = '#0d1f0d';
    row.style.outline = '1px solid #1a7a3c';
    var msg = document.getElementById('saveMsg');
    if (msg) msg.style.display = 'none';
}

function addRow() {
    document.getElementById('mapBody').insertAdjacentHTML('beforeend', buildRow(rowCount, rowCount, '', ''));
    markRowDirty(document.getElementById('mapBody').lastElementChild);
    rowCount++;
}

function removeRow(i) {
    var row = document.getElementById('row_'+i);
    if (row) row.remove();
    reindexRows();
    // Renumber LED# values to stay contiguous after deletion
    document.getElementById('mapBody').querySelectorAll('tr').forEach(function(r, idx) {
        var ledInp = r.querySelector('input.led-input');
        if (ledInp) ledInp.value = idx;
        markRowDirty(r);
    });
}

function doImport(append) {
    var raw = document.getElementById('importArea').value;
    var parsed = [];
    var re = /\{\s*(\d+)\s*,\s*"([^"]*)"\s*,\s*"([^"]*)"\s*\}/g;
    var m;
    while ((m = re.exec(raw)) !== null) {
        parsed.push({ led: parseInt(m[1], 10), name: m[2], city: m[3] });
    }
    if (parsed.length === 0) { alert('Could not parse any entries.\nExpected format:\n{ 0, "Name", "City"}, { 1, "Name", "City"}'); return; }
    parsed.sort(function(a, b) { return a.led - b.led; });
    var tbody = document.getElementById('mapBody');
    if (!append) {
        tbody.innerHTML = '';
        rowCount = 0;
    } else {
        // Sync rowCount with actual number of rows before appending
        rowCount = tbody.querySelectorAll('tr').length;
    }
    parsed.forEach(function(p) {
        tbody.insertAdjacentHTML('beforeend', buildRow(rowCount, p.led, p.name, p.city));
        markRowDirty(tbody.lastElementChild);
        rowCount++;
    });
    resolveCollisions();
    sortTableByLed();
    document.getElementById('importArea').value = '';
}

// ---- Resolve duplicate LED# values across all rows ----
// Iterates inputs sorted by current value; any duplicate is bumped up by 1
// repeatedly until it finds a free slot.
function resolveCollisions() {
    var inputs = Array.from(document.querySelectorAll('#mapBody input.led-input'));
    inputs.sort(function(a, b) { return parseInt(a.value, 10) - parseInt(b.value, 10); });
    var seen = {};
    inputs.forEach(function(inp) {
        var v = parseInt(inp.value, 10);
        while (seen[v] !== undefined) { v++; }
        if (v !== parseInt(inp.value, 10)) {
            inp.value = v;
            markRowDirty(inp.closest('tr'));
        }
        seen[v] = true;
    });
}

// ---- LED# change handler: resolve collisions then sort ----
function onLedChanged(inp) {
    markRowDirty(inp.closest('tr'));
    var newVal = parseInt(inp.value, 10);
    if (isNaN(newVal) || newVal < 0) { inp.value = 0; newVal = 0; }
    if (newVal > 255) { inp.value = 255; newVal = 255; }

    // Cascade: if newVal is taken by another input, bump that one up
    var inputs = Array.from(document.querySelectorAll('#mapBody input.led-input'));
    var changed = true;
    while (changed) {
        changed = false;
        var seen = {};
        inputs.forEach(function(other) {
            var v = parseInt(other.value, 10);
            if (other === inp) return;
            if (seen[v] !== undefined) {
                // bump this duplicate up by 1
                other.value = v + 1;
                changed = true;
            }
            seen[v] = other;
        });
        // also check against inp's current value
        inputs.forEach(function(other) {
            if (other === inp) return;
            if (parseInt(other.value, 10) === newVal) {
                other.value = parseInt(other.value, 10) + 1;
                changed = true;
            }
        });
        // re-read inputs after changes
        inputs = Array.from(document.querySelectorAll('#mapBody input.led-input'));
    }

    sortTableByLed();
    reindexRows();
}

// ---- Sort table rows by LED# value ----
function sortTableByLed() {
    var tbody = document.getElementById('mapBody');
    var rows = Array.from(tbody.querySelectorAll('tr'));
    rows.sort(function(a, b) {
        var aVal = parseInt(a.querySelector('input.led-input').value, 10);
        var bVal = parseInt(b.querySelector('input.led-input').value, 10);
        return aVal - bVal;
    });
    rows.forEach(function(r) { tbody.appendChild(r); });
    reindexRows();
}

// ---- Drag & Drop reorder ----
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
    if (target !== dragSrc) {
        target.style.borderTop = '2px solid #1a7a3c';
    }
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
        if (fromIdx < toIdx) {
            tbody.insertBefore(dragSrc, target.nextSibling);
        } else {
            tbody.insertBefore(dragSrc, target);
        }
        // Reassign all LED numbers by position after drag and mark dirty
        var allRows = Array.from(tbody.querySelectorAll('tr'));
        allRows.forEach(function(r, idx) {
            var ledInp = r.querySelector('input.led-input');
            if (ledInp) ledInp.value = idx;
            markRowDirty(r);
        });
        reindexRows();
    }
}

function onDragEnd(e) {
    e.currentTarget.style.opacity = '';
    document.querySelectorAll('#mapBody tr').forEach(function(r) {
        r.style.borderTop = '';
    });
}

function doExport() {
    var rows = document.getElementById('mapBody').querySelectorAll('tr');
    var parts = [];
    rows.forEach(function(r) {
        var led  = r.querySelector('input.led-input').value;
        var name = r.querySelectorAll('input')[1].value;
        var city = r.querySelectorAll('input')[2].value;
        if (name.length > 0) {
            parts.push('{ ' + led + ', "' + name + '", "' + city + '"}');
        }
    });
    var content = parts.join(', ') + '\n';
    var blob = new Blob([content], { type: 'text/plain' });
    var a = document.createElement('a');
    a.href = URL.createObjectURL(blob);
    a.download = 'searchList.txt';
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(a.href);
}

function reindexRows() {
    var rows = document.getElementById('mapBody').querySelectorAll('tr');
    rowCount = 0;
    rows.forEach(function(r) {
        r.id = 'row_' + rowCount;
        var inputs = r.querySelectorAll('input');
        var types = ['led_', 'name_', 'city_'];
        inputs.forEach(function(inp, j) { inp.name = types[j] + rowCount; });
        var btn = r.querySelector('button');
        if (btn) btn.setAttribute('onclick', 'removeRow(' + rowCount + ')');
        rowCount++;
    });
}
</script>
)rawjs";

    return html + htmlFooter();
}

// Helper: one table row for the SpaceMap editor.
String SettingsWebServer::buildSpaceMapRow(int i, int led, const String& name, const String& city) {
    return "<tr id='row_" + String(i) + "' draggable='true' ondragstart='onDragStart(event)' ondragover='onDragOver(event)' ondrop='onDrop(event)' ondragend='onDragEnd(event)' style='cursor:grab'>"
           "<td style='padding:4px;width:24px;color:#555;font-size:1.2em;text-align:center;cursor:grab' title='Drag to reorder'>&#8597;</td>"
           "<td style='padding:4px'><input type='number' name='led_"  + String(i) + "' value='" + String(led)  + "' min='0' max='255' style='width:60px;background:#1e1e1e;color:#eee;border:1px solid #3a3a3a;border-radius:4px;padding:4px;-moz-appearance:textfield' class='led-input' onchange='onLedChanged(this)'></td>"
           "<td style='padding:4px'><input type='text'   name='name_" + String(i) + "' value='" + name        + "' style='width:100%;background:#1e1e1e;color:#eee;border:1px solid #3a3a3a;border-radius:4px;padding:4px' oninput='markRowDirty(this.closest(\"tr\"))'></td>"
           "<td style='padding:4px'><input type='text'   name='city_" + String(i) + "' value='" + city        + "' style='width:100%;background:#1e1e1e;color:#eee;border:1px solid #3a3a3a;border-radius:4px;padding:4px' oninput='markRowDirty(this.closest(\"tr\"))'></td>"
           "<td style='padding:4px'><button type='button' onclick='removeRow(" + String(i) + ")' style='background:#f5a800;color:#fff;border:none;border-radius:4px;padding:4px 10px;cursor:pointer'>&#10005;</button></td>"
           "</tr>";
}