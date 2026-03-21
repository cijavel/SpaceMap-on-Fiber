#include "DataSpaceList.h"

// --------------------------------------------------------------------------
// Static list of hackerspaces to track.
// Each entry maps a LED strip index to a space name and city.
// Commented-out entries are prepared but currently not wired to a physical LED.
// --------------------------------------------------------------------------
SpaceSearchList searchList[] = {
    { 0, "OpenLab Augsburg"                  , "Augsburg"},
    { 1, "IT-Syndikat"                       , "Innsbruck"},
    { 2, "MuCCC"                             , "Munich"},
    { 3, "realraum"                          , "Graz"},
    { 4, "Binary Kitchen"                    , "Regensburg"},
    { 5, "c-base"                            , "Berlin"},
    { 6, "Chaosdorf"                         , "Dusseldorf"},
    { 7, "CCC Frankfurt"                     , "Frankfurt"},
    { 8, "backspace"                         , "Bamberg"},
    { 9, "CCCHH"                             , "Hamburg" },
    {10, "vspace.one"                        , "VS-Villingen"},
    {11, "Nerdberg"                          , "Fuerth"},
    {12, "dezentrale"                        , "Leipzig"},
   //{13, "shackspace - stuttgart hackerspace", "Stuttgart"},
   //{14, "temporaerhaus"                     , "Ulm"},
   //{15, "Chaostreff Bern"                   , "Bern"},
   //{16, "hacKNology e.V."                   , "Konstanz"},
   //{17, "CCCFr"                             , "Freiburg im Breisgau"},
   //{18, "turmlabor"                         , "Dresden"},
   //{19, "Eigenbaukombinat Halle e.V."       , "Halle (Saale)"},
   //{20, "Krautspace - Hackspace Jena e.V."  , "Jena"},
   //{21, "flipdot"                           , "Kassel"},
   //{22, "CCC Frankfurt"                     , "Frankfurt"},
   //{23, "Chaostreff Dortmund"               , "Dortmund"},
   //{24, "Hackerspace Bremen e.V."           , "Bremen"},
   //{25, "HSBXL"                             , "Brussels"},
};

// --------------------------------------------------------------------------
// Returns the LED index for the hackerspace with the given name.
// Returns -1 if the name is not found in the watch list.
// --------------------------------------------------------------------------
int DataSpaceList::getLEDforName(String name) {
    for (const auto& entry : searchList) {
        if (entry.getName() == name) {
            return entry.getLED();
        }
    }
    return -1;
}

int DataSpaceList::getNumberofSpacesonwatch() {
    return sizeof(searchList) / sizeof(searchList[0]);
}
