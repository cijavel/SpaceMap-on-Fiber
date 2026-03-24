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
    void handleSettingsPost(AsyncWebServerRequest* request);
    void handleSettingsReset(AsyncWebServerRequest* request);
    void handleSpaceMapGet(AsyncWebServerRequest* request);
    void handleSpaceMapPost(AsyncWebServerRequest* request);
    void handleSpaceMapReset(AsyncWebServerRequest* request);
    void handleApiSpaceMapGet(AsyncWebServerRequest* request);
    void handleSpaceMapExport(AsyncWebServerRequest* request);
    void handleSpaceMapBlink(AsyncWebServerRequest* request);
    void handleNotFound(AsyncWebServerRequest* request);
    void handleHttpsRedirect(AsyncWebServerRequest* request);

    // Streaming page renderers — write directly into an AsyncResponseStream,
    // never build a monolithic String in heap.
    static void streamIndexPage(AsyncResponseStream* s, const String& message = "");
    static void streamSettingsPage(AsyncResponseStream* s, const String& message = "");
    static void streamSpaceMapPage(AsyncResponseStream* s, const String& message = "");

    // Shared fragment helpers (still return small Strings — all well under 512 B).
    static String htmlHeader(const String& title);
    static String htmlFooter();
    static String navBar();
};

#endif // SETTINGS_WEB_SERVER_H
