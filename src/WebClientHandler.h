#ifndef SPACE_API_ON_FIBER_WEBCLIENTHANDLER_H
#define SPACE_API_ON_FIBER_WEBCLIENTHANDLER_H

#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "Configuration.h"
#include <ArduinoJson.h>
#include "DataStructure.h"
#include <vector>
#include "DataSpaceList.h"
#include "TimeHandler.h"

class WebClientHandler {
public:
    // All methods are static – no instance needed.
    static void getSpaceStatus(std::vector<SpaceStatusList> &spaStaVector,
                               const String &webpageout);

private:
    WebClientHandler() = delete; // purely static class – no instantiation

    static void modifyStatus(std::vector<SpaceStatusList> &spaStaVector,
                             int led, const String &name, SpaceStatus status);
};

#endif // SPACE_API_ON_FIBER_WEBCLIENTHANDLER_H
