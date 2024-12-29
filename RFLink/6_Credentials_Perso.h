/////////////////////////////////////////////
//!! Credentials Personnels - gitignored !!//
/////////////////////////////////////////////

#include "RFLink.h"

#ifndef CREDENTIALS_PERSO_h
#define CREDENTIALS_PERSO_h

String FiltreMac(String Mac);

#ifdef WIFIMANAGER_ENABLED
#define DeviceName DEFAULT_WIFI_CLIENT_HOSTNAME
#define WiFiManager_SSID DeviceName
#define WiFiManager_IP 192,168,4,1
#define WiFiManager_Gateway 192,168,4,1
#define WiFiManager_SubNet 255,255,255,0
#endif

#ifdef FOTA_ENABLED
#define FOTA_FILE "http://idkonnect.fr/FirmWare/" DeviceName ".bin"
#endif

// MQTT Server
    #undef MQTT_SERVER
    #undef MQTT_PORT
    #undef MQTT_ID
    #undef MQTT_USER
    #undef MQTT_PSWD
#define MQTT_SERVER "www.idkonnect.fr"
#define MQTT_PORT 1883
#define MQTT_ID (String(DeviceName "_")+FiltreMac(WiFi.macAddress())).c_str()
#define MQTT_USER (String(DeviceName "_")+FiltreMac(WiFi.macAddress())).c_str()
#define MQTT_PSWD ""
// MQTT Server
    #undef MQTT_TOPIC_OUT
    #undef MQTT_TOPIC_IN
    #undef MQTT_TOPIC_LWT
#define MQTT_TOPIC_OUT (String("msg/" DeviceName "/")+FiltreMac(WiFi.macAddress())).c_str()
#define MQTT_TOPIC_IN (String("cmnd/" DeviceName "/")+FiltreMac(WiFi.macAddress())+String("/#")).c_str()
#define MQTT_TOPIC_LWT (String("msg/" DeviceName "/")+FiltreMac(WiFi.macAddress())+String("/LWT")).c_str()
// MQTT Topic : MQTT_TOPIC_IN must end with '/#' to read eQ3 commands

// OTA
#undef AutoOTA_URL
#define AutoOTA_URL FOTA_FILE

/*
#ifdef CHECK_CACERT
static const char ca_cert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIID (...)
-----END CERTIFICATE-----
)EOF";
#endif //CHECK_CACERT
*/

#endif