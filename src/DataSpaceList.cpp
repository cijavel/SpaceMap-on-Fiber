#include "DataSpaceList.h"

// --------------------------------------------------------------------------
// Hackerspaces of interest
// --------------------------------------------------------------------------
// LED No, Name for parsing
SpaceSearchList searchList[] = {
    { 1, "OpenLab Augsburg"                   }, 
    { 2, "/usr/space"                         }, 
    { 3, "Chaos Computer Club Wien (C3W)"     }, 
    { 4, "not on api: Fortisauris Slovak"     }, 
    { 5, "base48"                             },
    { 6, "IT-Syndikat"                        }, 
    { 7, "MuCCC"                              }, 
    { 8, "realraum"                           }, 
    { 9, "Binary Kitchen"                     }
   
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






