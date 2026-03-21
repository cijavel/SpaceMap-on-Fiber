#include "SettingsWebServer.h"
#include "AppConfig.h"
#include "Configuration.h"
#include <ArduinoJson.h>

// --------------------------------------------------------------------------
// Öffentliche Schnittstelle
// --------------------------------------------------------------------------
void SettingsWebServer::begin() {
    registerRoutes();
    _server.begin();
#ifdef DEBUG
    Serial.println("WEB: Settings server started on port 80");
#endif
}

// --------------------------------------------------------------------------
// Routen registrieren
// --------------------------------------------------------------------------
void SettingsWebServer::registerRoutes() {
    // Index
    _server.on("/", HTTP_GET, [this](AsyncWebServerRequest* req) {
        handleIndex(req);
    });

    // Settings: GET zeigt Formular, POST speichert
    _server.on("/settings", HTTP_GET, [this](AsyncWebServerRequest* req) {
        handleSettingsGet(req);
    });

    _server.on("/settings", HTTP_POST,
        [this](AsyncWebServerRequest* req) { /* onRequest – wird nach Body aufgerufen */ },
        nullptr,
        [this](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
            handleSettingsPost(req, data, len, index, total);
        }
    );

    // Reset auf Defaults
    _server.on("/settings/reset", HTTP_POST, [this](AsyncWebServerRequest* req) {
        handleSettingsReset(req);
    });

    _server.onNotFound([this](AsyncWebServerRequest* req) {
        handleNotFound(req);
    });
}

// --------------------------------------------------------------------------
// Route-Handler
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
    if (index == 0) request->_tempObject = new String();
    String& body = *reinterpret_cast<String*>(request->_tempObject);
    body += String((const char*)data, len);

    if (index + len < total) return; // noch nicht vollständig – _tempObject wird
    // vom ESPAsyncWebServer bei Abbruch NICHT automatisch freigegeben.
    // onDisconnect-Cleanup ist hier nicht möglich ohne Request-Lifetime-Tracking.
    // Stattdessen: onBody wird nur bei vollständigem Request mit total==len aufgerufen,
    // daher sicherer Reset durch expliziten Guard:
    if (!request->_tempObject) return;

    AppConfig& cfg = AppConfig::getInstance();

    // URL-encoded form-Daten parsen
    auto getValue = [&](const String& key) -> String {
        int start = body.indexOf(key + "=");
        if (start == -1) return "";
        start += key.length() + 1;
        int end = body.indexOf("&", start);
        if (end == -1) end = body.length();
        String val = body.substring(start, end);
        // URL-Decode: %XX und + durch Space ersetzen
        val.replace("+", " ");
        String decoded = "";
        for (int i = 0; i < (int)val.length(); i++) {
            if (val[i] == '%' && i + 2 < (int)val.length()) {
                char hex[3] = { val[i+1], val[i+2], 0 };
                decoded += (char)strtol(hex, nullptr, 16);
                i += 2;
            } else {
                decoded += val[i];
            }
        }
        return decoded;
    };

    // Werte einlesen und setzen
    String apiUrl = getValue("apiUrl");
    if (apiUrl.length() > 0)           cfg.setSpaceApiUrl(apiUrl);

    String wifiInt = getValue("wifiInterval");
    if (wifiInt.length() > 0)          cfg.setIntervalWifiCheck(wifiInt.toInt());

    String ledInt = getValue("ledInterval");
    if (ledInt.length() > 0)           cfg.setIntervalLEDs(ledInt.toInt());

    String apiInt = getValue("apiInterval");
    if (apiInt.length() > 0)           cfg.setIntervalApi(apiInt.toInt());

    String ledBright = getValue("ledBrightness");
    if (ledBright.length() > 0)        cfg.setLedBrightness((uint8_t)ledBright.toInt());

    String obBright = getValue("onboardBrightness");
    if (obBright.length() > 0)         cfg.setOnboardBrightness((uint8_t)obBright.toInt());

    String maxPwr = getValue("ledMaxPower");
    if (maxPwr.length() > 0)           cfg.setLedMaxPowerMa((uint16_t)maxPwr.toInt());

    cfg.save();

    // Temporären Body-Puffer freigeben
    if (request->_tempObject) {
        delete reinterpret_cast<String*>(request->_tempObject);
        request->_tempObject = nullptr;
    }

    #ifdef DEBUG
        Serial.println("WEB: Settings saved via web interface");
    #endif

        request->send(200, "text/html", buildSettingsPage("Einstellungen gespeichert."));
}

void SettingsWebServer::handleSettingsReset(AsyncWebServerRequest* request) {
    AppConfig::getInstance().resetToDefaults();
    #ifdef DEBUG
        Serial.println("WEB: Settings reset to defaults");
    #endif
        request->send(200, "text/html", buildSettingsPage("Einstellungen auf Standardwerte zurückgesetzt."));
}

void SettingsWebServer::handleNotFound(AsyncWebServerRequest* request) {
    request->send(404, "text/html",
        htmlHeader("404") +
        "<h2>404 – Seite nicht gefunden</h2>"
        "<p><a href='/'>Zurück zur Startseite</a></p>" +
        htmlFooter());
}

// --------------------------------------------------------------------------
// HTML-Aufbau
// --------------------------------------------------------------------------
String SettingsWebServer::htmlHeader(const String& title) {
    return String(
        "<!DOCTYPE html><html lang='de'><head>"
        "<meta charset='UTF-8'>"
        "<meta name='viewport' content='width=device-width, initial-scale=1'>"
        "<title>SpaceMap – ") + title + "</title>"
        "<style>"
        "body{font-family:sans-serif;margin:0;background:#111;color:#eee;}"
        "nav{background:#222;padding:10px 20px;}"
        "nav a{color:#4af;text-decoration:none;margin-right:16px;font-weight:bold;}"
        "nav a:hover{text-decoration:underline;}"
        ".container{max-width:700px;margin:30px auto;padding:0 16px;}"
        "h1,h2{color:#4af;}"
        "label{display:block;margin-top:14px;font-size:0.9em;color:#aaa;}"
        "input[type=text],input[type=number],input[type=url]"
        "{width:100%;padding:7px;background:#222;color:#eee;border:1px solid #444;"
        "border-radius:4px;box-sizing:border-box;margin-top:4px;}"
        ".btn{display:inline-block;margin-top:20px;padding:9px 22px;"
        "border:none;border-radius:4px;cursor:pointer;font-size:1em;}"
        ".btn-save{background:#4af;color:#111;}"
        ".btn-reset{background:#c44;color:#fff;margin-left:12px;}"
        ".msg{margin-top:14px;padding:10px;background:#1a3a1a;border-left:4px solid #4a4;"
        "border-radius:4px;color:#8f8;}"
        ".msg-reset{background:#3a1a1a;border-left:4px solid #c44;color:#f88;}"
        "</style></head><body>";
}

String SettingsWebServer::htmlFooter() {
    return "</div></body></html>";
}

String SettingsWebServer::navBar() {
    return "<nav>"
           "<a href='/'>&#127968; Übersicht</a>"
           "<a href='/settings'>&#9881; Einstellungen</a>"
           "</nav><div class='container'>";
}

String SettingsWebServer::buildIndexPage(const String& message) {
    String html = htmlHeader("Übersicht") + navBar();
    html += "<h1>SpaceMap on Fiber</h1>";
    html += "<p>Willkommen auf der Weboberfläche des SpaceMap-Controllers.</p>";
    html += "<ul>"
            "<li><a href='/settings'>&#9881; Einstellungen bearbeiten</a></li>"
            "</ul>";
    if (message.length() > 0) {
        html += "<div class='msg'>" + message + "</div>";
    }
    return html + htmlFooter();
}

String SettingsWebServer::buildSettingsPage(const String& message) {
    AppConfig& cfg = AppConfig::getInstance();
    String html = htmlHeader("Einstellungen") + navBar();
    html += "<h1>Einstellungen</h1>";

    if (message.length() > 0) {
        bool isReset = message.indexOf("zurückgesetzt") >= 0;
        html += "<div class='msg" + String(isReset ? " msg-reset" : "") + "'>" + message + "</div>";
    }

    html += "<form method='POST' action='/settings'>";

    html += "<h2>API</h2>";
    html += "<label>SpaceAPI URL</label>"
            "<input type='url' name='apiUrl' value='" + cfg.getSpaceApiUrl() + "'>";
    html += "<label>API-Abfrageintervall (Sekunden)</label>"
            "<input type='number' name='apiInterval' min='10' max='3600' value='" +
            String(cfg.getIntervalApi()) + "'>";

    html += "<h2>WLAN</h2>";
    html += "<label>WLAN-Prüfintervall (Sekunden)</label>"
            "<input type='number' name='wifiInterval' min='30' max='86400' value='" +
            String(cfg.getIntervalWifiCheck()) + "'>";

    html += "<h2>LEDs</h2>";
    html += "<label>LED-Aktualisierungsintervall (Sekunden)</label>"
            "<input type='number' name='ledInterval' min='1' max='3600' value='" +
            String(cfg.getIntervalLEDs()) + "'>";
    html += "<label>LED-Helligkeit (0–255)</label>"
            "<input type='number' name='ledBrightness' min='0' max='255' value='" +
            String(cfg.getLedBrightness()) + "'>";
    html += "<label>Onboard-LED-Helligkeit (0–255)</label>"
            "<input type='number' name='onboardBrightness' min='0' max='255' value='" +
            String(cfg.getOnboardBrightness()) + "'>";
    html += "<label>Max. Stromverbrauch LEDs (mA)</label>"
            "<input type='number' name='ledMaxPower' min='100' max='5000' value='" +
            String(cfg.getLedMaxPowerMa()) + "'>";
            
    html += "<br><button type='submit' class='btn btn-save'>&#128190; Speichern</button>";
    html += "</form>";

    html += "<form method='POST' action='/settings/reset' "
            "onsubmit=\"return confirm('Wirklich auf Standardwerte zurücksetzen?')\">"
            "<button type='submit' class='btn btn-reset'>&#8635; Auf Standard zurücksetzen</button>"
            "</form>";

    return html + htmlFooter();
}