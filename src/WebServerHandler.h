#ifndef SPACE_API_ON_FIBER_WEBSERVERHANDLER_H
#define SPACE_API_ON_FIBER_WEBSERVERHANDLER_H


#include "ESPAsyncWebServer.h"
#include <vector>
#include "DataStructure.h"
#include "DataSpaceList.h"

class WebServerHandler {
public:
    static WebServerHandler &getInstance() {
        static WebServerHandler instance; // Guaranteed to be destroyed.
        return instance;// Instantiated on first use.
    }
    void start();
    void setData(std::vector<SpaceStatusList> &spacestatus, unsigned long currentSeconds);
    static String getStatusColor(int color);
private:
    AsyncWebServer server = AsyncWebServer(80);
    static void handle_index(AsyncWebServerRequest *request);
    static void handle_NotFound(AsyncWebServerRequest *request);
    WebServerHandler();                    // Constructor? (the {} brackets) are needed here.
    WebServerHandler(WebServerHandler const&);  // Don't Implement
    void operator=(WebServerHandler const&); // Don't implement
};
#endif //SPACE_API_ON_FIBER_WEBSERVERHANDLER_H
