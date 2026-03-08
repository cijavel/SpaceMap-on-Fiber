//uncomment to get debug in serial
//#define DEBUG 1

#define interval_in_Seconds_WiFiCheck 300
#define WIFI_CONNECT_TIMEOUT_STEPS 20   // 20 × 500ms = 10 Sekunden pro Versuch
#define WIFI_MAX_RETRIES 5              // max. 5 Versuche beim Start
#define WIFI_MAX_FAILED_RECONNECTS 10   // Reboot nach 10 fehlgeschlagenen Reconnects
#define interval_in_Seconds_LEDs 10
#define interval_in_Seconds_api 120
#define interval_in_Seconds_RAMPrintout 30

#define webpage_SpaceAPI "https://spaceapi.ccc.de/api/spaces"

#define BAUDRATE 9600
#define DeviceName "SpaceMap on Fiber 2023"

#define LED_BRIGHTNESS 255
#define ONBOARD_BRIGHTNESS 10
#define LED_COUNT 30
#define LED_DATA_PIN 14

#define LED_MAX_POWER_MILLIAMPS 500
#define LED_TYPE WS2812
#define LED_COLOR_ORDER RGB
