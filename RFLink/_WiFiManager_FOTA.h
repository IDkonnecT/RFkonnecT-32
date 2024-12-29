// ************************************* //
// * Arduino Project RFLink-esp        * //
// * https://github.com/couin3/RFLink  * //
// * WiFiManager & FOTA Functionality  * //
// * Proposed by Thierry - IDkonnecT   * //
// ************************************* //

/*
MQTT_XXX    = default value
MQTT_xxx    = current value
mqtt_xxx    = wifiManager parameter
*/

#ifndef _WiFiManager_FOTA_h
#define _WiFiManager_FOTA_h

#include <Arduino.h>

#if defined(WIFIMANAGER_ENABLED) || defined(FOTA_ENABLED)
    #include <Preferences.h>
    void Debug(String Texte, boolean EndLine);
#endif 


#ifdef WIFIMANAGER_ENABLED

    #include <WiFiManager.h>

    void WiFiManagerWiFiFail(WiFiManager *myWiFiManager);
    void WiFiManagerConfigOK();
    void WiFiManagerParamOK();
    void WiFiManagerPortalTimeOut();
    void SetUpWiFiManager();
    void WiFiManagerPortal();
    void ConfigJSONlittleFS(String JSONconfig);
    void EcraseCredentialsMQTTduJSONlittleFS();

#endif // WIFIMANAGER_ENABLED


#ifdef FOTA_ENABLED

    #include <HTTPClient.h>
    #include <HTTPUpdate.h>

    void Fota();

#endif // FOTA_ENABLED


#endif // _WiFiManager_FOTA_h