#ifndef SPACE_API_ON_FIBER_DATASPACEAPI_H
#define SPACE_API_ON_FIBER_DATASPACEAPI_H

#include <Arduino.h>
#include "DataStructure.h"
#include <ArduinoJson.h>

class DataSpaceApi {

    private:
            DataSpaceApi() {};                   // Constructor? (the {} brackets) are needed here.
            DataSpaceApi(DataSpaceApi const&);   // Don't Implement
            void operator=(DataSpaceApi const&); // Don't implement
    public:

        static DataSpaceApi &getInstance() {
            static DataSpaceApi instance; // Guaranteed to be destroyed.
            return instance;// Instantiated on first use.
        };

        int getLEDforName(String name);
       
};
#endif //SPACE_API_ON_FIBER_DATASPACEAPI_H

    
