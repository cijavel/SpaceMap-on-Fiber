SpaceMap on Fiber
SpaceMap on Fiber is an ESP32-based device that visualizes the open/closed status of Hackerspaces on an LED strip using the SpaceAPI.


Features:

Downloads and parses the SpaceAPI JSON to retrieve the current open/closed/unknown status of each Hackerspace
Displays the status on a WS2812 LED strip – one pixel per Hackerspace (green = open, red = closed, blue = unknown)
Onboard RGB LED indicates device status: blue during API calls, green when connected, red when WiFi is unavailable
LED startup sequence to verify strip functionality on boot
Automatic WiFi reconnect with reboot watchdog for reliable long-term operation
