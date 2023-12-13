#ifndef SPACE_API_ON_FIBER_WEBCLIENTHANDLER_H
#define SPACE_API_ON_FIBER_WEBCLIENTHANDLER_H


#include <HTTPClient.h>
#include "Configuration.h"
#include <ArduinoJson.h>
#include "DataStructure.h"
#include <vector>
#include "DataSpaceList.h"
#include "TimeHandler.h"


class WebClientHandler {
public:


    static std::vector<SpaceStatusList> getSpaceStatus(String webpage, unsigned long currentSeconds) ;
    static WebClientHandler &getInstance() {
            static WebClientHandler instance; // Guaranteed to be destroyed.
            return instance;// Instantiated on first use.
    };
private:
    WebClientHandler() {};                   // Constructor? (the {} brackets) are needed here.
    WebClientHandler(WebClientHandler const&);   // Don't Implement
    void operator=(WebClientHandler const&); // Don't implement
    static void modifyStatus(std::vector<SpaceStatusList> &spaStaVector, int led, String name, SpaceStatus status);


};

#endif //SPACE_API_ON_FIBER_WEBCLIENTHANDLER_H