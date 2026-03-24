## Projektbeschreibung (KI-Format)
```
PROJECT: SpaceMap on Fiber
PLATFORM: ESP32 (Arduino framework, PlatformIO)
PURPOSE: Displays open/closed status of hackerspaces on a physical NeoPixel LED strip,
         fetched from a SpaceAPI-compatible JSON endpoint.

ARCHITECTURE:
  main.cpp              — Setup/loop, interval scheduling (WiFi watchdog, API poll, LED update).
                          Holds the global spaceStatusList (vector<SpaceStatusList>) that is
                          shared with SettingsWebServer via extern declaration.
                          On setup(), performs one immediate API fetch before entering loop().
  AppConfig             — Singleton. Loads/saves runtime settings (intervals, brightness, URL)
                          via ESP32 NVS (Preferences). Compile-time defaults in Configuration.h.
  DataSpaceList         — Singleton. Holds the LED↔Hackerspace mapping. Lazy-loaded from NVS;
                          falls back to built-in default list (DataSpaceList.cpp).
                          Always LED_COUNT (50) slots; empty entries (name="") render as black LEDs.
                          Max 64 entries (SPACEMAP_MAX_ENTRIES).
  NeoPixelLED           — Singleton. Wraps NeoPixelBus strip. Thread-safe via FreeRTOS mutex.
                          Methods: initLEDs, enumerateLEDs (startup sequence),
                          updateLEDs / updateLEDsUnsafe (apply status colors),
                          blinkLED (locate single LED by blinking white 10×, then restores full strip).
  WebClientHandler      — Fetches SpaceAPI JSON over HTTPS (WiFiClientSecure, setInsecure),
                          streams and parses response object-by-object to keep heap usage low.
                          Tracks last HTTP code, attempt timestamp, found/parse/total counts,
                          and unmatched space names for the web UI.
  WiFiHandler           — Connects to WiFi on startup; reconnects if lost; reboots after
                          WIFI_MAX_FAILED_RECONNECTS failed attempts.
  TimeHandler           — Syncs NTP time (used for status change timestamps).
  SettingsWebServer     — Singleton. AsyncWebServer on port 80. Routes:
                            GET  /                  — Overview page: live API health + parse status
                                                       (JS polls /api/status every 30 s)
                            GET  /settings          — Settings form (intervals, brightness, API URL)
                            POST /settings          — Save settings to NVS
                            POST /settings/reset    — Reset settings to compile-time defaults
                            GET  /spacemap          — Hackerspace mapping editor. Fixed LED_COUNT rows,
                                                       LED# read-only label; drag to swap positions;
                                                       import (replace by position or next free slot);
                                                       export includes all slots incl. empty ones.
                                                       Status column shows ⏳ until first ESP parse,
                                                       then OPEN/CLOSED/UNKNOWN/— per space.
                                                       Auto-refreshes status every 15 s and immediately
                                                       when the ESP completes a new API fetch
                                                       (detected via ageSec reset in /api/status).
                            POST /spacemap          — Save all LED_COUNT slots (incl. empty) to NVS
                            POST /spacemap/reset    — Restore built-in default mapping
                            GET  /spacemap/blink    — Blink single LED in a dedicated FreeRTOS task
                                                       (param: led=<index>); restores strip afterwards
                            GET  /spacemap/export   — Download all LED_COUNT slots as .txt (incl. empty)
                            GET  /api/status        — JSON: httpCode, ok, ageSec, url, foundCount,
                                                       parseErrors, totalObjects, watchListSize, unmatched[]
                            GET  /api/spacestatus   — JSON array: [{name, status, led}, …] for all
                                                       currently tracked spaces with known status
                          Port 443 (HTTPS) redirects to HTTP.

DATA FLOW:
  1. Loop polls SpaceAPI every `intervalApi` seconds → WebClientHandler fills spaceStatusList
  2. Loop refreshes LED strip every `intervalLEDs` seconds → NeoPixelLED::updateLEDs()
  3. Each SpaceStatusList entry: LED index (int), name (String), status (OPEN/CLOSED/UNKNOWN),
     lastChange timestamp (String)
  4. Status colors: OPEN=green(0,255,0), CLOSED=red(255,0,0), UNKNOWN=blue(24,12,128)
     Brightness is scaled per-channel at runtime.

LED STATUS (onboard RGB_BUILTIN):
  BLUE  — API call in progress
  GREEN — WiFi connected (idle)
  RED   — WiFi disconnected

NVS NAMESPACES:
  "spacemap" — App settings (wifiInterval, ledInterval, apiInterval, apiUrl, ledBright, obBright, ledMaxPwr)
  "smdata"   — SpaceMap entries (smCount, smL0…smLN, smN0…smNN, smC0…smCN)

KNOWN ISSUES:
  1. blinkLED uses delay(150) × 20 = 3 s of blocking calls inside a FreeRTOS task.
     The task runs at priority 1 and holds the strip mutex for the full duration,
     blocking LED updates from loop() during that window.

CONFIGURATION (Configuration.h):
  LED_COUNT=50, LED_DATA_PIN=14, LED_BRIGHTNESS=255, ONBOARD_BRIGHTNESS=10
  API interval=120s, LED refresh=10s, WiFi check=300s
  Default SpaceAPI URL: https://api.spaceapi.io/
```
