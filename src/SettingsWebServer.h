#ifndef SETTINGS_WEB_SERVER_H
#define SETTINGS_WEB_SERVER_H

#include <ESPAsyncWebServer.h>

// Kapselt den AsyncWebServer und registriert alle Routen
// für die Web-Einstellungsseiten.
class SettingsWebServer {
public:
    static SettingsWebServer& getInstance() {
        static SettingsWebServer instance;
        return instance;
    }

    // Server starten und Routen registrieren. Einmalig in setup() aufrufen.
    void begin();

private:
    SettingsWebServer() : _server(80) {}
    SettingsWebServer(const SettingsWebServer&) = delete;
    void operator=(const SettingsWebServer&) = delete;

    AsyncWebServer _server;

    void registerRoutes();

    // Route-Handler
    void handleIndex(AsyncWebServerRequest* request);
    void handleSettingsGet(AsyncWebServerRequest* request);
    void handleSettingsPost(AsyncWebServerRequest* request,
                            uint8_t* data, size_t len,
                            size_t index, size_t total);
    void handleSettingsReset(AsyncWebServerRequest* request);
    void handleNotFound(AsyncWebServerRequest* request);

    // Hilfsmethoden zum HTML-Aufbau
    static String buildIndexPage(const String& message = "");
    static String buildSettingsPage(const String& message = "");
    static String htmlHeader(const String& title);
    static String htmlFooter();
    static String navBar();
};

#endif // SETTINGS_WEB_SERVER_H