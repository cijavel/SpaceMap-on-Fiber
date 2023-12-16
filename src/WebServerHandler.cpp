#include "WebServerHandler.h"
#include "Configuration.h"
#include "DataStructure.h"
#include "DataSpaceList.h"
#include "DataSpaceList.h"

static std::vector<SpaceStatusList> dataspacestatus;
DataSpaceList &listofspaces= DataSpaceList::getInstance();



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


    html += "<h1>System status</h1>";
    html += "<p>";
    html += "<div>webpage:         " + String(webpage_SpaceAPI) + "</div>";
    html += "<div>Leds in System:  " + String(LED_COUNT) + "</div>";
    html += "<div>Spaces on Watch: " + String(listofspaces.getNumberofSpacesonwatch()) + "</div>";
    html += "<div>Spaces found:    " + String(dataspacestatus.size()) + "</div>";
    html += "</p>";
    html += "<hr>";  



    html += "<h1>Space Data</h1>";

    // Iterate through the vector and add each entry to the HTML
    for (auto& data : dataspacestatus) {
      html += "<div style=' color:" + getStatusColor(data.getStatus()) +   ";'>";
      html += "<div>LED No: " + String(data.getLED()) + "</div>";
      html += "<div>space: " + String(data.getName()) + "</div>";
      html += "<div>status: <span style='font-weight: bold;'> " + String(data.getStatus()) + "</span></div>";
      html += "<div>time:   " + String(data.getlastChange()) + "</div>";
      html += "</div>";
      html += "<p></p>";
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

String WebServerHandler::getStatusColor(int color) {


      switch ( color )
      {
         case 1:
            return "green";
            break;
         case 2:
            return "red";
            break;
         case 3:
            return "blue";
            break;
         default:
            break;
      }

    return "black";
}

