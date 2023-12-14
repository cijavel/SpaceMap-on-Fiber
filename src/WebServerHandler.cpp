#include "WebServerHandler.h"
#include "Configuration.h"
#include "DataStructure.h"
#include "DataSpaceList.h"

static std::vector<SpaceStatusList> dataspacestatus;

// --------------------------------------------------------------------------
// set up page
// --------------------------------------------------------------------------
WebServerHandler::WebServerHandler()
{
  server.on("/", HTTP_GET, handle_index);
  server.onNotFound(handle_NotFound);
}


// --------------------------------------------------------------------------
// index webpage
// --------------------------------------------------------------------------
void WebServerHandler::handle_index(AsyncWebServerRequest *request)
{
    String html = "<html><body>";
    html += "<h1>Space Data</h1>";

    // Iterate through the vector and add each entry to the HTML
    for (auto& data : dataspacestatus) {
      html += "<p>LED No: " + String(data.getLED()) + "</p>";
      html += "<p>status: " + String(data.getStatus()) + "</p>";
      html += "<p>time: "   + String(data.getlastChange()) + "</p>";
      html += "<p>space: "  + String(data.getName()) + "</p>";
      html += "<hr>";  // Add a horizontal line between entries
    }

    html += "</body></html>";
    request->send(200, "text/html; charset=utf-8", html);
  
}

// --------------------------------------------------------------------------
// notfound page
// --------------------------------------------------------------------------
void WebServerHandler::handle_NotFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain; charset=utf-8", "Not found");
}

// --------------------------------------------------------------------------
// set data for the webpage
// --------------------------------------------------------------------------
void WebServerHandler::setData(std::vector<SpaceStatusList> &spacestatus, unsigned long currentSeconds){
    if (currentSeconds % interval_in_Seconds_webserver == 0){  
      if (!spacestatus.empty()) {
        dataspacestatus = spacestatus;
      }
    }

}

// --------------------------------------------------------------------------
// initial webserver
// --------------------------------------------------------------------------
void WebServerHandler::start()
{
  server.begin();
}
