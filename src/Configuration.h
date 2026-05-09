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

// 115200 chosen so that Serial.print() in the API/WiFi error paths does not
// noticeably block the loop (60 bytes at 9600 baud = ~60 ms; at 115200 ~5 ms).
#define BAUDRATE 115200
#define DeviceName "SpaceMap on Fiber 2026"

// LED strip configuration.
// LED_SLOT_COUNT defines both the number of physical LEDs on the strip
// and the number of LED<->Hackerspace mapping slots.
#define LED_SLOT_COUNT 60
#define LED_BRIGHTNESS 255
#define ONBOARD_BRIGHTNESS 10
#define LED_DATA_PIN 14

// Safety cap on total LED strip current draw in milliamps.
#define LED_MAX_POWER_MILLIAMPS 500
#define LED_TYPE WS2812
#define LED_COLOR_ORDER RGB
