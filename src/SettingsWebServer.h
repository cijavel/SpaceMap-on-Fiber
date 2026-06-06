#ifndef SETTINGS_WEB_SERVER_H
#define SETTINGS_WEB_SERVER_H

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <atomic>

// Stack size (bytes) for the blink FreeRTOS task.
// The BlinkParams snapshot vector lives on the task stack — needs more than default.
static constexpr uint32_t kBlinkTaskStackSize = 4096;

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
    // NOTE: this listens on port 443 as *plain-text* HTTP, NOT HTTPS. It only
    // serves a best-effort meta-refresh redirect for browsers that try
    // https://<device-ip>. Real TLS clients fail the handshake before they
    // ever reach a handler — that is expected for this LAN-only device.
    AsyncWebServer _port443Redirect{443};

    // True while a /spacemap/blink task is active. Prevents concurrent blink
    // tasks from flooding the heap and the FreeRTOS task pool.
    // Atomic because the value is read/written from the AsyncWebServer task
    // and the FreeRTOS blink task on different cores.
    static std::atomic<bool> _blinkTaskRunning;

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

    // Streaming HTML fragment helpers — write directly into the response
    // stream so the CSS block from PROGMEM is never copied into a heap String.
    static void streamHtmlHeader(AsyncResponseStream* s, const String& title);
    static void streamHtmlFooter(AsyncResponseStream* s);
    static void streamNavBar(AsyncResponseStream* s);

    // Escaping helpers — sanitise user-supplied strings before embedding in HTML/JSON.
    // htmlAttrEscape: escapes &, <, >, ", ' for use in HTML attribute values or text nodes.
    static String htmlAttrEscape(const String& in);
    // jsonEscape: escapes \, ", and control characters (\n \r \t) for JSON string literals.
    static String jsonEscape(const String& in);
};

#endif // SETTINGS_WEB_SERVER_H
