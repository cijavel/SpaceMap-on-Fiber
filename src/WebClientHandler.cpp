#include "WebClientHandler.h"


void WebClientHandler::modifyStatus( std::vector<SpaceStatusList> &spaStaVector, int led, String name, SpaceStatus status) {

    bool change = false;
    for(auto &element : spaStaVector ){
      if(element.getName() != name){
        continue;
      } 
      change = true; 
      element.setStatus(status);
      element.setlastChange(TimeHandler::localTime("%Y.%m.%d %H:%M"));
      break;
    }

  if (change == false){
     spaStaVector.push_back({led, name, status , TimeHandler::localTime("%Y.%m.%d %H:%M")});
  }

}  


std::vector<SpaceStatusList> WebClientHandler::getSpaceStatus(std::vector<SpaceStatusList> &spaceStatusVector,String webpageout, unsigned long currentSeconds) {
    DataSpaceList &SpaceBase = DataSpaceList::getInstance();

    int spaceledNr;
    String spaceName;
    String currentSpaceStatus;

    HTTPClient http; 

    if (currentSeconds % interval_in_Seconds_Json == 0){

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

      DynamicJsonDocument doc(1024);   // You can use a String as your JSON input.WARNING: the string in the input  will be duplicated in the JsonDocument.

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
    }
    http.end();
    return spaceStatusVector;
}


