#include "WebClientHandler.h"

// --------------------------------------------------------------------------
// add a new Hackerpace to vectore or change the status of an existing hackerspace
// --------------------------------------------------------------------------
void WebClientHandler::modifyStatus( std::vector<SpaceStatusList> &spaStaVector, int led, String name, SpaceStatus status) {

    bool change = false;
    for(auto &element : spaStaVector ){
      if(element.getName() != name){
        continue;
      } 
      change = true; 
      
      if (element.getStatus() != status) {
        element.setStatus(status);
        element.setlastChange(TimeHandler::localTime("%Y.%m.%d %H:%M"));
      }

      break;
    }

  if (change == false){
     spaStaVector.push_back({led, name, status , TimeHandler::localTime("%Y.%m.%d %H:%M")});
  }

}  

// --------------------------------------------------------------------------
// download and parse json from spaceAPI. Call to set up status list 
// --------------------------------------------------------------------------
std::vector<SpaceStatusList> WebClientHandler::getSpaceStatus(std::vector<SpaceStatusList> &spaceStatusVector, String webpageout) {
   

    DataSpaceList &SpaceBase = DataSpaceList::getInstance();
    int spaceledNr;
    String spaceName;
    String currentSpaceStatus;
    HTTPClient http; 

    
    //set working status to BLUE
    neopixelWrite(RGB_BUILTIN ,0,0,ONBOARD_BRIGHTNESS); // BLUE
    // Create an empty array to hold Space objects
    WiFiClientSecure client;

    // configure server and url
    client.setInsecure();
    http.begin(client, webpageout );
    http.useHTTP10(true); // important for chunking and stream reading
    int httpCode = http.GET();
    Stream& payload = http.getStream();

    StaticJsonDocument<128> filter;
    filter["url"] = true;
    filter["space"] = true;
    filter["state"]["open"] = true;
    filter["state"]["lastchange"] = true;

    DynamicJsonDocument doc(4096);  // You can use a String as your JSON input.WARNING: the string in the input  will be duplicated in the JsonDocument.

    payload.find("["); // should actually be byte 0 of the response stream
    do {

        spaceledNr = -1;
        DeserializationError error = deserializeJson(doc, payload, DeserializationOption::Filter(filter));
        if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.c_str());
        }
        else
        {
            // if space is in known_spaces, update status
            spaceName = doc["space"].as<String>();
            spaceledNr = SpaceBase.getLEDforName(spaceName);
            currentSpaceStatus = doc["state"]["open"].as<String>();

              if (-1 < spaceledNr) {
                if (currentSpaceStatus == "true") {
                  modifyStatus(spaceStatusVector, spaceledNr, spaceName,SpaceStatus::OPEN);  
                } else if (currentSpaceStatus == "false") {
                  modifyStatus(spaceStatusVector, spaceledNr, spaceName,SpaceStatus::CLOSED);  
                } else {
                  modifyStatus(spaceStatusVector, spaceledNr, spaceName,SpaceStatus::UNKNOWN);  

                }
              }
        }
    } while (payload.findUntil(",","]"));
    http.end();
    
    

    //WLAN Status back to GREEN/RED
    if (WiFiClass::status() == WL_CONNECTED)
    {
        neopixelWrite(RGB_BUILTIN ,0,ONBOARD_BRIGHTNESS,0); // GREEN
    }
    else
    {
        neopixelWrite(RGB_BUILTIN ,ONBOARD_BRIGHTNESS,0,0); // RED
    }

    
    return spaceStatusVector;
}


