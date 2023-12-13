#include "DataSpaceApi.h"


SpaceSearchList searchList[] = {
    { 0, "OpenLab Augsburg"                   }, 
    { 1, "/usr/space"                         }, 
    { 2, "Chaos Computer Club Wien (C3W)"     }, 
    { 3, "not on api: Fortisauris Slovak"     }, 
    { 4, "base48"                             }, 
    { 5, "Warsaw Hackerspace"                 }, 
    { 6, "Salzburg"                           }, 
    { 7, "DevLoL"                             }, 
    { 8, "Polytechnischer Werkraum Zittau"    }, 
    { 9, "C3D2"                               }, 
    { 10, "Chaostreff Potsdam (CCCP)"         }, 
    { 11, "xHain"                             }, 
    { 12, "IT-Syndikat"                       }, 
    { 13, "MuCCC"                             }, 
    { 14, "realraum"                          }, 
    { 15, "Binary Kitchen"                    }, 
    { 16, "Nerdberg"                          }, 
    { 17, "backspace"                         }, 
    { 18, "hackzogtum"                        }, 
    { 19, "Krautspace"                        }, 
    { 20, "Eigenbaukombinat Halle e.V."       }, 
    { 21, "Netz39"                            }, 
    { 22, "Stratum 0"                         }, 
    { 23, "Hacklabor"                         }, 
    { 24, "coredump"                          },
    { 25, "see-base"                          },
    { 26, "shackspace - stuttgart hackerspace"},
    { 27, "mag.lab"                           },
    { 28, "flipdot"                           },
    { 29, "CCCHH"                             },
    { 30, "Chaos Computer Club Basel"         },
    { 31, "CCCFr"                             },
    { 32, "Entropia"                          },
    { 33, "RaumZeitLabor"                     },
    { 34, "CCC Frankfurt"                     },
    { 35, "[hsmr] Hackspace Marburg"          },
    { 36, "Hackerspace Bielefeld e.V."        },
    { 37, "Hackerspace Bremen e.V."           },
    { 38, "Level2"                            },
    { 39, "Maschinendeck"                     },
    { 40, "Westwoodlabs"                      },
    { 41, "CCC Cologne"                       },
    { 42, "Chaosdorf"                         },
    { 43, "chaospott"                         },
    { 44, "Chaostreff Dortmund"               },
    { 45, "warpzone "                         },
    { 46, "TkkrLab"                           },
    { 47, "Hackerspace Drenthe"               },
    { 48, "FUZ"                               },
    { 49, "HSBXL"                             },
    { 50, "RevSpace"                          },
    { 51, "Bitlair"                           },
    { 52, "Frack"                             },
    { 53, "not on api: Fortisauris Slovak"    },
    { 54, "not on api: HSPSH"                 },
    { 55, "not in api: liege hackspace"       }
};

int DataSpaceApi::getLEDforName(String name) {
    int result = -1;
    for (const auto& item : searchList) {
        if (item.getName() == name) {
                result =  item.getLED();
                break;
        }
    }
    return result;

}






