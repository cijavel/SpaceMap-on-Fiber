#ifndef SPACE_API_ON_FIBER_DATASPACELIST_H
#define SPACE_API_ON_FIBER_DATASPACELIST_H

#include <Arduino.h>
#include "DataStructure.h"
#include <ArduinoJson.h>

class DataSpaceList {

    private:
            DataSpaceList() {};                   // Constructor? (the {} brackets) are needed here.
            DataSpaceList(DataSpaceList const&);   // Don't Implement
            void operator=(DataSpaceList const&); // Don't implement
    public:

        static DataSpaceList &getInstance() {
            static DataSpaceList instance; // Guaranteed to be destroyed.
            return instance;// Instantiated on first use.
        };

        int getLEDforName(String name);
       
};
#endif //SPACE_API_ON_FIBER_DATASPACELIST_H

    
