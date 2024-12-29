#ifndef _10_WiFi_H
#define _10_WiFi_H

#include "RFLink.h"

#ifdef RFLINK_WIFI_ENABLED
//*** IDkonnecT >>>
#ifndef WIFIMANAGER_ENABLED
//<<< IDkonnecT ***
#ifdef ESP8266
#include "ESP8266WiFi.h"
#else
#include "WiFi.h"
#endif
#endif

#include "11_Config.h"

namespace RFLink {
    namespace Wifi {

        extern Config::ConfigItem configItems[];

        void setup();
        void mainLoop();
        
        void stop_WIFI();
        void start_WIFI();

        void resetClientWifi(); // to connect/reconnect client wifi after settings have changed

        void clientParamsUpdatedCallback();
        void accessPointParamsUpdatedCallback();
        void reconnectServices();

        bool clientNetworkIsUp();
        bool ntpIsSynchronized();

        void getStatusJsonString(JsonObject &output);
    }

    namespace AutoOTA {
        void checkForUpdateAndApply();
    }
}

//*** IDkonnecT >>>
#endif //WIFIMANAGER_ENABLED
//<<< IDkonnecT ***

#endif //_10_WiFi_H

