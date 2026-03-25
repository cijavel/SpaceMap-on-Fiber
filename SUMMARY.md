## Projektbeschreibung (KI-Format)
```
PROJECT: SpaceMap on Fiber
PLATFORM: ESP32-S3 (board: esp32-s3-devkitc-1, Arduino framework, PlatformIO)
PURPOSE: Displays open/closed status of hackerspaces on a physical NeoPixel LED strip,
         fetched from a SpaceAPI-compatible JSON endpoint.

LIBRARIES:
  bblanchon/ArduinoJson, makuna/NeoPixelBus,
  esp32async/AsyncTCP, esp32async/ESPAsyncWebServer

ARCHITECTURE:
  main.cpp              — Setup/loop, interval scheduling (WiFi watchdog, API poll, LED update).
                          Holds the global spaceStatusList (vector<SpaceStatusList>) that is
                          shared with SettingsWebServer via extern declaration.
                          On setup(), performs one immediate API fetch before entering loop().
  AppConfig             — Singleton. Loads/saves runtime settings (intervals, brightness, URL,
                          max LED power) via ESP32 NVS (Preferences). Compile-time defaults in
                          Configuration.h. Also handles SpaceMap persistence directly via a
                          dedicated _smPrefs handle (namespace "smdata") to avoid open/close
                          conflicts with the settings _prefs handle (namespace "spacemap").
                          Methods: load, save, resetToDefaults,
                          loadSpaceMap, saveSpaceMap, resetSpaceMap.
  DataSpaceList         — Singleton. Holds the LED↔Hackerspace mapping. Lazy-loaded from NVS
                          via AppConfig; falls back to built-in default list (DataSpaceList.cpp,
                          13 entries). The web UI always displays LED_COUNT (50) fixed rows;
                          empty entries (name="") render as black LEDs.
                          Each entry carries a `disabled` flag; disabled LEDs are forced off
                          regardless of space status. Max LED_COUNT entries (SPACEMAP_LED_MAX);
                          heap-allocated at runtime.
  NeoPixelLED           — Singleton. Wraps NeoPixelBus strip. Thread-safe via FreeRTOS mutex.
                          Methods: initLEDs, enumerateLEDs (startup sequence),
                          updateLEDs / updateLEDsUnsafe (apply status colors; skips disabled
                          entries, forcing those LEDs off), blinkLED (locate single LED by
                          blinking white 10×, then restores full strip),
                          validateLEDIndices (range check before applying updates).
  WebClientHandler      — Purely static class (never instantiated). Fetches SpaceAPI JSON over
                          HTTPS (WiFiClientSecure, setInsecure), streams and parses response
                          object-by-object to keep heap usage low.
                          Tracks last HTTP code, attempt timestamp, found/parse/total counts,
                          and unmatched space names for the web UI.
  WiFiHandler           — Connects to WiFi on startup; reconnects if lost; reboots after
                          WIFI_MAX_FAILED_RECONNECTS failed attempts.
  TimeHandler           — Syncs NTP time (used for status change timestamps).
  SettingsWebServer     — Singleton. AsyncWebServer on port 80. Large HTML/CSS/JS blocks stored
                          in PROGMEM and JSON API responses streamed directly into
                          AsyncResponseStream — never built as a monolithic heap String. Routes:
                            GET  /                  — Overview page: live API health + parse status
                                                       (JS polls /api/status every 30 s)
                            GET  /settings          — Settings form (intervals, brightness,
                                                       API URL, max LED power)
                            POST /settings          — Save settings to NVS
                            POST /settings/reset    — Reset settings to compile-time defaults
                            GET  /spacemap          — Hackerspace mapping editor. Fixed LED_COUNT
                                                       rows, LED# read-only label; drag to swap
                                                       positions; per-entry disable checkbox
                                                       (forces LED off on next update, row shown
                                                       in italics); import (replace by position or
                                                       next free slot); export includes all slots
                                                       incl. empty ones.
                                                       Status column shows ⏳ until first ESP parse,
                                                       then OPEN/CLOSED/UNKNOWN/— per space.
                                                       Auto-refreshes status every 15 s and
                                                       immediately when the ESP completes a new API
                                                       fetch (detected via ageSec reset in
                                                       /api/status).
                            POST /spacemap          — Save all LED_COUNT slots (incl. empty) to NVS
                            POST /spacemap/reset    — Restore built-in default mapping
                            GET  /spacemap/blink    — Blink single LED in a dedicated FreeRTOS task
                                                       (param: led=<index>); restores strip
                                                       afterwards
                            GET  /spacemap/export   — Download all LED_COUNT slots as .txt
                                                       (incl. empty)
                            GET  /api/spacemap       — JSON array of all LED_COUNT slots
                                                       [{led, name, city, disabled}, …];
                                                       consumed by /spacemap JS to lazy-load the
                                                       editor table and avoid heap pressure on
                                                       the ESP32 during initial page render
                            GET  /api/status        — JSON: httpCode, ok, ageSec, url, foundCount,
                                                       parseErrors, totalObjects, watchListSize,
                                                       unmatched[]
                            GET  /api/spacestatus   — JSON array: [{name, status, led}, …] for all
                                                       currently tracked spaces (status mapped to
                                                       OPEN/CLOSED/UNKNOWN; INIT rendered as
                                                       UNKNOWN)
                          Port 443 (HTTPS) redirects to HTTP.

DATA FLOW:
  1. Loop polls SpaceAPI every `intervalApi` seconds → WebClientHandler fills spaceStatusList
  2. Loop refreshes LED strip every `intervalLEDs` seconds → NeoPixelLED::updateLEDs()
  3. Each SpaceStatusList entry: LED index (int), name (String), status (SpaceStatus enum:
     INIT/OPEN/CLOSED/UNKNOWN), lastChange timestamp (String)
  4. Status colors: OPEN=green(0,255,0), CLOSED=red(255,0,0), UNKNOWN=blue(24,12,128)
     Brightness is scaled per-channel at runtime. INIT treated as UNKNOWN.

LED STATUS (onboard RGB_BUILTIN):
  BLUE  — API call in progress
  GREEN — WiFi connected (idle)
  RED   — WiFi disconnected

NVS NAMESPACES:
  "spacemap" — App settings (wifiInterval, ledInterval, apiInterval, apiUrl,
                ledBright, obBright, ledMaxPwr)
  "smdata"   — SpaceMap entries (smCount, smL0…smLN, smN0…smNN, smC0…smCN, smD0…smDN)

KNOWN ISSUES:
  1. blinkLED uses delay(150) × 20 = 3 s of blocking calls inside a FreeRTOS task.
     The task runs at priority 1 and holds the strip mutex for the full duration,
     blocking LED updates from loop() during that window.

CONFIGURATION (Configuration.h):
  DeviceName="SpaceMap on Fiber 2026"
  BAUDRATE=9600
  LED_COUNT=50, LED_DATA_PIN=14, LED_BRIGHTNESS=255, ONBOARD_BRIGHTNESS=10
  LED_MAX_POWER_MILLIAMPS=500, LED_TYPE=WS2812, LED_COLOR_ORDER=RGB
  API interval=120s, LED refresh=10s, WiFi check=300s
  WIFI_CONNECT_TIMEOUT_STEPS=20 (× 500 ms = 10 s per attempt)
  WIFI_MAX_RETRIES=5 (startup), WIFI_MAX_FAILED_RECONNECTS=10 (triggers reboot)
  Default SpaceAPI URL: https://api.spaceapi.io/
```
