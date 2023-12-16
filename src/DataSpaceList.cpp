#include "DataSpaceList.h"

// --------------------------------------------------------------------------
// Hackerspaces of interest
// --------------------------------------------------------------------------
// LED No, Name for parsing
SpaceSearchList searchList[] = {
    { 0, "OpenLab Augsburg"                   }, 
    { 1, "IT-Syndikat"                        }, 
    { 2, "MuCCC"                              }, 
    { 3, "realraum"                           }, 
    { 4, "Binary Kitchen"                     }
};

// --------------------------------------------------------------------------
// Get the Led number for the Hackerspace
// --------------------------------------------------------------------------
int DataSpaceList::getLEDforName(String name) {
    int result = -1;
    for (const auto& item : searchList) {
        if (item.getName() == name) {
                result =  item.getLED();
                break;
        }
    }
    return result;
}


int DataSpaceList::getNumberofSpacesonwatch() {
    return sizeof(searchList) / sizeof(searchList[0]);
}

        


