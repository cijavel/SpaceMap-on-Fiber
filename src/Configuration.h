// Uncomment to enable debug output on the serial port.
//#define DEBUG 1

// WiFi watchdog interval in seconds (checked periodically in the main loop).
#define interval_in_Seconds_WiFiCheck 300

// Number of 500 ms wait steps before a single WiFi connect attempt times out (20 × 500 ms = 10 s).
#define WIFI_CONNECT_TIMEOUT_STEPS 20

// Maximum number of WiFi connect attempts during startup before rebooting.
#define WIFI_MAX_RETRIES 5

// Number of consecutive failed reconnects before the device reboots.
#define WIFI_MAX_FAILED_RECONNECTS 10

// How often (in seconds) the LED strip is refreshed with the latest space status.
#define interval_in_Seconds_LEDs 10

// How often (in seconds) the SpaceAPI endpoint is polled for fresh data.
#define interval_in_Seconds_api 120

// How often (in seconds) RAM usage is printed to serial (debug builds only).
#define interval_in_Seconds_RAMPrintout 30

// Default SpaceAPI endpoint – returns a JSON array of all registered hackerspaces.
#define webpage_SpaceAPI "https://api.spaceapi.io/"

#define BAUDRATE 9600
#define DeviceName "SpaceMap on Fiber 2026"

// LED strip configuration.
#define LED_BRIGHTNESS 255
#define ONBOARD_BRIGHTNESS 10
#define LED_COUNT 50
#define LED_DATA_PIN 14

// Safety cap on total LED strip current draw in milliamps.
#define LED_MAX_POWER_MILLIAMPS 500
#define LED_TYPE WS2812
#define LED_COLOR_ORDER RGB

// Maximum number of LED<->Hackerspace mapping slots.
// Tied to LED_COUNT so the mapping always covers the full physical strip.
#define SPACEMAP_LED_MAX LED_COUNT
