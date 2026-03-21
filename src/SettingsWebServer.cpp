#include "SettingsWebServer.h"
#include "AppConfig.h"
#include "Configuration.h"
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

    // onRequest fires after the body is received; actual parsing happens in the body callback.
    _server.on("/settings", HTTP_POST,
        [this](AsyncWebServerRequest* req) { /* placeholder – body handler runs first */ },
        nullptr,
        [this](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
            handleSettingsPost(req, data, len, index, total);
        }
    );

    _server.on("/settings/reset", HTTP_POST, [this](AsyncWebServerRequest* req) {
        handleSettingsReset(req);
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

void SettingsWebServer::handleSettingsPost(AsyncWebServerRequest* request,
                                           uint8_t* data, size_t len,
                                           size_t index, size_t total) {
    // Accumulate body chunks into a String stored in _tempObject.
    if (index == 0) request->_tempObject = new String();
    String& body = *reinterpret_cast<String*>(request->_tempObject);
    body += String((const char*)data, len);

    if (index + len < total) return; // Body not yet complete – wait for more chunks.
    if (!request->_tempObject) return;

    AppConfig& cfg = AppConfig::getInstance();

    // Parse a single URL-encoded form field from the request body.
    auto getValue = [&](const String& key) -> String {
        int start = body.indexOf(key + "=");
        if (start == -1) return "";
        start += key.length() + 1;
        int end = body.indexOf("&", start);
        if (end == -1) end = body.length();

        String encodedValue = body.substring(start, end);

        // URL-decode: replace '+' with space and '%XX' sequences with their characters.
        encodedValue.replace("+", " ");
        String decodedValue = "";
        for (int i = 0; i < (int)encodedValue.length(); i++) {
            if (encodedValue[i] == '%' && i + 2 < (int)encodedValue.length()) {
                char hex[3] = { encodedValue[i+1], encodedValue[i+2], 0 };
                decodedValue += (char)strtol(hex, nullptr, 16);
                i += 2;
            } else {
                decodedValue += encodedValue[i];
            }
        }
        return decodedValue;
    };

    // Helper: apply a parsed unsigned long only if it is within [minVal, maxVal].
    auto applyULong = [](const String& s, unsigned long minVal, unsigned long maxVal,
                         std::function<void(unsigned long)> setter) {
        if (s.length() == 0) return;
        long value = s.toInt();
        if (value > 0 && (unsigned long)value >= minVal && (unsigned long)value <= maxVal)
            setter((unsigned long)value);
    };

    // Helper: apply a parsed uint8_t only if it is within [0, 255].
    auto applyUInt8 = [](const String& s, std::function<void(uint8_t)> setter) {
        if (s.length() == 0) return;
        int value = s.toInt();
        if (value >= 0 && value <= 255) setter((uint8_t)value);
    };

    // Helper: apply a parsed uint16_t only if it is within [minVal, maxVal].
    auto applyUInt16 = [](const String& s, uint16_t minVal, uint16_t maxVal,
                          std::function<void(uint16_t)> setter) {
        if (s.length() == 0) return;
        int value = s.toInt();
        if (value >= minVal && value <= maxVal) setter((uint16_t)value);
    };

    String apiUrl = getValue("apiUrl");
    if (apiUrl.length() > 0) cfg.setSpaceApiUrl(apiUrl);

    applyULong(getValue("wifiInterval"),     30,   86400, [&](unsigned long value){ cfg.setIntervalWifiCheck(value); });
    applyULong(getValue("ledInterval"),       1,    3600, [&](unsigned long value){ cfg.setIntervalLEDs(value); });
    applyULong(getValue("apiInterval"),      10,    3600, [&](unsigned long value){ cfg.setIntervalApi(value); });
    applyUInt8(getValue("ledBrightness"),              [&](uint8_t value){ cfg.setLedBrightness(value); });
    applyUInt8(getValue("onboardBrightness"),          [&](uint8_t value){ cfg.setOnboardBrightness(value); });
    applyUInt16(getValue("ledMaxPower"), 100,  5000, [&](uint16_t value){ cfg.setLedMaxPowerMa(value); });

    cfg.save();

    // Release the temporary body buffer.
    if (request->_tempObject) {
        delete reinterpret_cast<String*>(request->_tempObject);
        request->_tempObject = nullptr;
    }

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
        "nav a{color:#f5a800;text-decoration:none;margin-right:16px;font-weight:bold;}"
        "nav a:hover{color:#fff;text-decoration:underline;}"
        ".container{max-width:700px;margin:30px auto;padding:0 16px;}"
        "h1{color:#f5a800;}"
        "h2{color:#e02020;}"
        "label{display:block;margin-top:14px;font-size:0.9em;color:#bbb;}"
        "input[type=text],input[type=number],input[type=url]"
        "{width:100%;padding:7px;background:#1e1e1e;color:#eee;border:1px solid #3a3a3a;"
        "border-radius:4px;box-sizing:border-box;margin-top:4px;}"
        "input:focus{outline:2px solid #f5a800;border-color:#f5a800;}"
        ".btn{display:inline-block;margin-top:20px;padding:10px 24px;"
        "border:none;border-radius:5px;cursor:pointer;font-size:1em;font-weight:bold;}"
        ".btn-save{background:#f5a800;color:#111;}"
        ".btn-save:hover{background:#ffc030;}"
        ".btn-reset{background:#e02020;color:#fff;margin-left:12px;}"
        ".btn-reset:hover{background:#ff3a3a;}"
        ".btn-settings{display:inline-block;margin-top:20px;padding:10px 24px;"
        "background:#1a7a3c;color:#fff;border:none;border-radius:5px;cursor:pointer;"
        "font-size:1em;font-weight:bold;text-decoration:none;}"
        ".btn-settings:hover{background:#24a050;}"
        ".msg{margin-top:14px;padding:10px;background:#0e2e0e;border-left:4px solid #1a7a3c;"
        "border-radius:4px;color:#6fcf6f;}"
        ".msg-reset{background:#2e0e0e;border-left:4px solid #e02020;color:#f88;}"
        "</style></head><body>";
}

String SettingsWebServer::htmlFooter() {
    return "</div></body></html>";
}

String SettingsWebServer::navBar() {
    return "<nav>"
           "<a href='/'>&#127968; Overview</a>"
           "<a href='/settings'>&#9881; Settings</a>"
           "</nav><div class='container'>";
}

String SettingsWebServer::buildIndexPage(const String& message) {
    String html = htmlHeader("Overview") + navBar();
    html += "<h1>SpaceMap on Fiber</h1>";
    html += "<p>Welcome to the SpaceMap controller web interface.</p>";
    html += "<a href='/settings' class='btn-settings'>&#9881; Edit Settings</a>";
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
