#ifndef SETTINGS_WEB_SERVER_H
#define SETTINGS_WEB_SERVER_H

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>

// Wraps the AsyncWebServer and registers all routes for the settings web UI.
// Access via getInstance(); call begin() once in setup().
class SettingsWebServer {
public:
    static SettingsWebServer& getInstance() {
        static SettingsWebServer instance;
        return instance;
    }

    // Start the server and register all routes. Call once in setup().
    void begin();

private:
    SettingsWebServer() : _server(80) {}
    SettingsWebServer(const SettingsWebServer&) = delete;
    void operator=(const SettingsWebServer&) = delete;

    AsyncWebServer _server;
    AsyncWebServer _httpsServer{443};

    void registerRoutes();

    // Route handlers
    void handleIndex(AsyncWebServerRequest* request);
    void handleSettingsGet(AsyncWebServerRequest* request);
    void handleSettingsPost(AsyncWebServerRequest* request,
                            uint8_t* data, size_t len,
                            size_t index, size_t total);
    void handleSettingsReset(AsyncWebServerRequest* request);
    void handleNotFound(AsyncWebServerRequest* request);
    void handleHttpsRedirect(AsyncWebServerRequest* request);

    // HTML page builders
    static String buildIndexPage(const String& message = "");
    static String buildSettingsPage(const String& message = "");
    static String buildSpaceMapPage(const String& message = "");
    static String htmlHeader(const String& title);
    static String htmlFooter();
    static String navBar();
};

#endif // SETTINGS_WEB_SERVER_H
