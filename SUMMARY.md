
## Projektbeschreibung (KI-Format)
```
PROJECT: SpaceMap on Fiber
PLATFORM: ESP32 (Arduino framework, PlatformIO)
PURPOSE: Displays open/closed status of hackerspaces on a physical NeoPixel LED strip,
         fetched from a SpaceAPI-compatible JSON endpoint.

ARCHITECTURE:
  main.cpp              — Setup/loop, interval scheduling (WiFi watchdog, API poll, LED update)
  AppConfig             — Singleton. Loads/saves runtime settings (intervals, brightness, URL)
                          via ESP32 NVS (Preferences). Compile-time defaults in Configuration.h.
  DataSpaceList         — Singleton. Holds the LED↔Hackerspace mapping. Lazy-loaded from NVS;
                          falls back to built-in default list (DataSpaceList.cpp).
                          Max 64 entries (SPACEMAP_MAX_ENTRIES).
  NeoPixelLED           — Singleton. Wraps NeoPixelBus strip. Methods: initLEDs, enumerateLEDs
                          (startup sequence), updateLEDs (apply status colors), blinkLED (locate
                          single LED by blinking white 10×).
  WebClientHandler      — Fetches SpaceAPI JSON over HTTPS, parses it, fills spaceStatusList.
  WiFiHandler           — Connects to WiFi on startup; reconnects if lost; reboots after
                          WIFI_MAX_FAILED_RECONNECTS failed attempts.
  TimeHandler           — Syncs NTP time.
  SettingsWebServer     — Singleton. AsyncWebServer on port 80. Routes:
                            GET  /                  — Status page with live API health check (JS polling)
                            GET  /settings          — Settings form (intervals, brightness, API URL)
                            POST /settings          — Save settings to NVS
                            POST /settings/reset    — Reset settings to compile-time defaults
                            GET  /spacemap          — LED↔Space mapping editor (table, drag-sort, import/export)
                            POST /spacemap          — Save mapping to NVS
                            POST /spacemap/reset    — Restore built-in default mapping
                            GET  /spacemap/blink    — Blink single LED (param: led=<index>)
                            GET  /spacemap/export   — Download current mapping as .txt snippet
                            GET  /api/status        — JSON: last HTTP code, URL, age in seconds
                          Port 443 redirects to HTTP.

DATA FLOW:
  1. Loop polls SpaceAPI every `intervalApi` seconds → WebClientHandler returns vector<SpaceStatusList>
  2. Loop refreshes LED strip every `intervalLEDs` seconds → NeoPixelLED::updateLEDs()
  3. Each SpaceStatusList entry: LED index (uint8_t), name (String), status (OPEN/CLOSED/UNKNOWN)
  4. Status colors: OPEN=green(0,255,0), CLOSED=red(255,0,0), UNKNOWN=blue(24,12,128)
     Brightness is scaled per-channel at runtime.

LED STATUS (onboard RGB_BUILTIN):
  BLUE  — API call in progress
  GREEN — WiFi connected
  RED   — WiFi disconnected

NVS NAMESPACES:
  "spacemap" — App settings (wifiInterval, ledInterval, apiInterval, apiUrl, ledBright, obBright, ledMaxPwr)
  "smdata"   — SpaceMap entries (smCount, smL0…smLN, smN0…smNN, smC0…smCN)

KNOWN BUGS:
  1. handleSpaceMapBlink passes empty spaceStatusList to blinkLED → blinked LED stays OFF
     after blink instead of restoring space status color. Root cause: live spaceStatusList
     is a global in main.cpp, not accessible from SettingsWebServer.
  2. blinkLED uses delay(150) x20 = 3s blocking call inside ESPAsyncWebServer handler task.
     Blocks async_tcp task; no other HTTP requests served during blink.

CONFIGURATION (Configuration.h):
  LED_COUNT=30, LED_DATA_PIN=14, LED_BRIGHTNESS=255, ONBOARD_BRIGHTNESS=10
  API interval=120s, LED refresh=10s, WiFi check=300s
  Default SpaceAPI URL: https://api.spaceapi.io/