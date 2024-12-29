// ************************************* //
// * Arduino Project RFLink-esp        * //
// * https://github.com/couin3/RFLink  * //
// * WiFiManager & FOTA Functionality  * //
// * Proposed by Thierry - IDkonnecT   * //
// ************************************* //

#include <Arduino.h>
#include "RFLink.h"
#ifdef WIFIMANAGER_ENABLED

#include "_WiFiManager_FOTA.h"
#include "6_Credentials.h"
//#include <bootloader_random.h>

#include "6_MQTT.h"
#include "11_Config.h"

Preferences preferences;
WiFiManager wifiManager;
boolean SaveParameters=false;

String MQTT_server;
int MQTT_port;
String MQTT_id;
String MQTT_user;
String MQTT_pswd;
String MQTT_topic_out;
String MQTT_topic_in;
String MQTT_topic_lwt;
#ifdef FOTA_ENABLED
  String FOTA_file;
#endif

void Debug(String Texte="", boolean EndLine=true)
{
 if (EndLine) Serial.println(Texte);
 else Serial.print(Texte);
}

void ReadPreferences()
{
  preferences.begin(DeviceName, true);
  MQTT_server = preferences.getString("MQTT_SERVER", MQTT_SERVER);
  MQTT_port = preferences.getInt("MQTT_PORT", MQTT_PORT);
  MQTT_id = preferences.getString("MQTT_ID", MQTT_ID);
  MQTT_user = preferences.getString("MQTT_USER", MQTT_USER);
  MQTT_pswd  = preferences.getString("MQTT_PSWD", MQTT_PSWD);
  MQTT_topic_out = preferences.getString("MQTT_TOPIC_OUT", MQTT_TOPIC_OUT);
  MQTT_topic_in = preferences.getString("MQTT_TOPIC_IN", MQTT_TOPIC_IN);
  MQTT_topic_lwt = preferences.getString("MQTT_TOPIC_LWT", MQTT_TOPIC_LWT);
#ifdef FOTA_ENABLED
  FOTA_file = preferences.getString("FOTA_FILE", FOTA_FILE);
#endif
  preferences.end();
  Debug(F("MQTT preferences read :"));
  //Debug(F("***********************"));
  Debug("MQTT_SERVER= "+MQTT_server);
  Debug("MQTT_PORT= "+String(MQTT_port));
  Debug("MQTT_ID= "+MQTT_id);
  Debug("MQTT_USER= "+MQTT_user);
  Debug("MQTT_PSWD= "+MQTT_pswd);
  Debug("MQTT_TOPIC_OUT= "+MQTT_topic_out);
  Debug("MQTT_TOPIC_IN= "+MQTT_topic_in);
  Debug("MQTT_TOPIC_LWT= "+MQTT_topic_lwt);
#ifdef FOTA_ENABLED
  Debug("FOTA_FILE= "+FOTA_file);
#endif
}

void WiFiManagerWiFiFail(WiFiManager *myWiFiManager)
{
  Debug();
  Debug("********************************");
  Debug("Usual WiFi network not available");
  Debug("Reconfigure by connecting to WiFi : '", false);
  Debug(myWiFiManager->getConfigPortalSSID()+"' on page 'http://"+WiFi.softAPIP().toString()+"'");
  Debug("********************************");
  Debug();
}

void WiFiManagerConfigOK()
{
  Debug("Wifi Configuration saved !");
}

void WiFiManagerParamOK()
{
  SaveParameters = true;
}

void WiFiManagerPortalTimeOut()
{
  Debug("WiFi configuration ended by Timeout !");
  Debug("... REBOOT !!!");
  delay(1000);
  ESP.restart();
}

void SetUpWiFiManager()
{
  ReadPreferences();
  Debug();
	Debug("WiFi connexion ongoing...");
 
  //wifiManager.resetSettings();
  wifiManager.setDebugOutput(true);
  wifiManager.setEnableConfigPortal(true);
  wifiManager.setCaptivePortalEnable(true);   // If false, disable captive portal redirection
  wifiManager.setCountry("US"); // US, JP, CN (/!\ CN does not work in EU)
  wifiManager.setWiFiAPChannel(esp_random()%13+1);
  wifiManager.setHostname(DeviceName);   // If false, disable captive portal redirection
  // Menu setings
  wifiManager.setScanDispPerc(false);           // Affiche le RSSI en % ald BarreGraph
  //wifiManager.setClass("invert");             // Dark theme
  //wifiManager.setCustomHeadElement("<style>html{filter: invert(100%); -webkit-filter: invert(100%);}</style>"); // Custom HTML, CSS or Javascript 
  //WiFiManagerParameter custom_text("<p></p>");
    //wifiManager.addParameter(&custom_text);
#ifdef FOTA_ENABLED
  WiFiManagerParameter fota_file("FOTA", "Lien http pour Mise à Jour Firware (.bin)", FOTA_file.c_str(), 80); // Variable, description, défaut, longueur, style
    wifiManager.addParameter(&fota_file);
#endif
#ifndef FORCE_RESET_MQTT
  WiFiManagerParameter mqtt_server("Server", "Serveur MQTT", MQTT_server.c_str(), 40); // Variable, description, défaut, longueur, style
    wifiManager.addParameter(&mqtt_server);
  WiFiManagerParameter mqtt_port("Port", "Port MQTT", String(MQTT_port).c_str(), 5); // Variable, description, défaut, longueur, style
    wifiManager.addParameter(&mqtt_port);
  WiFiManagerParameter mqtt_id("ID", "ID MQTT", MQTT_id.c_str(), 32); // Variable, description, défaut, longueur, style
    wifiManager.addParameter(&mqtt_id);
  WiFiManagerParameter mqtt_user("User", "User MQTT", MQTT_user.c_str(), 32); // Variable, description, défaut, longueur, style
    wifiManager.addParameter(&mqtt_user);
  WiFiManagerParameter mqtt_pswd("Pswd", "PassWord MQTT", MQTT_pswd.c_str(), 32); // Variable, description, défaut, longueur, style
    wifiManager.addParameter(&mqtt_pswd);
  WiFiManagerParameter mqtt_topic_out("Topic_Out", "Topic_Out MQTT", MQTT_topic_out.c_str(), 64); // Variable, description, défaut, longueur, style
    wifiManager.addParameter(&mqtt_topic_out);
  WiFiManagerParameter mqtt_topic_in("Topic_In", "Topic_In MQTT", MQTT_topic_in.c_str(), 64); // Variable, description, défaut, longueur, style
    wifiManager.addParameter(&mqtt_topic_in);
  WiFiManagerParameter mqtt_topic_lwt("Topic_LWT", "Topic_LWT MQTT", MQTT_topic_lwt.c_str(), 64); // Variable, description, défaut, longueur, style
    wifiManager.addParameter(&mqtt_topic_lwt);
#endif
  //std::vector<const char *> menu = {"wifi","wifinoscan","info","param","close","sep","erase","restart","exit"};
  //std::vector<const char *> menu = {"wifi","param","sep"};
  std::vector<const char *> menu = {"wifi"};
  wifiManager.setMenu(menu);
  // Fine portal parameters
  wifiManager.setAPStaticIPConfig(IPAddress(WiFiManager_IP), IPAddress(WiFiManager_Gateway), IPAddress(WiFiManager_SubNet)); // IP, Gateway, SubNet
  wifiManager.setConnectRetries(10);   // 10s per trial, max = 10 trials
  //wifiManager.setConnectTimeout(30);  // Drawback => displays many line in Serial Monitor
  wifiManager.setConfigPortalTimeout(600);
  wifiManager.setWebPortalClientCheck(true);
  wifiManager.setMinimumSignalQuality(5);
  wifiManager.setAPCallback(WiFiManagerWiFiFail);
  wifiManager.setSaveConfigCallback(WiFiManagerConfigOK);
  wifiManager.setSaveParamsCallback(WiFiManagerParamOK);
  wifiManager.setConfigPortalTimeoutCallback(WiFiManagerPortalTimeOut); // Makes a reset if Portal times out, because a bug prevents normal WiFi reconnect after timeout
  wifiManager.setShowStaticFields(false);
  wifiManager.setShowDnsFields(false);
  // AutoReconnect (if not working, to be done done by WiFi.reconnect() in the main loop)
  //WiFi.setAutoConnect(true);    if (WiFi.getAutoConnect()) Debug(F("Regular WiFi autoConnect = YES")); else Debug(F("Regular WiFi autoConnect = NOK !!!!"));
  WiFi.setAutoReconnect(true);  if (WiFi.getAutoReconnect()) Debug(F("Regular WiFi autoReconnect = YES")); else Debug(F("Regular WiFi autoReconnect = NOK !!!!"));
  wifiManager.setRestorePersistent(true);
  wifiManager.setWiFiAutoReconnect(false);
  wifiManager.setCleanConnect(false);  // true => disconnect before connect => forget internal stored Wi-Fi credential
  // Launch (manual in case of Push Button)
  if(!wifiManager.autoConnect(WiFiManager_SSID)) Debug("Erreur WiFi !");
  /*
  preferences.begin(DeviceName);
  if (preferences.getBool("FORCE_PORTAL", false)==true)
    { if(!wifiManager.startConfigPortal(WiFiManager_SSID, "")) Debug("Erreur WiFi !"); }
  else
    { if(!wifiManager.autoConnect(WiFiManager_SSID, "")) Debug("Erreur WiFi !"); }
  preferences.putBool("FORCE_PORTAL", false);
  preferences.end();
  */
 
  // Save Parameters
#ifndef FORCE_RESET_MQTT
  if (SaveParameters)
    {
    // Préférences to Flash
    //https://github.com/espressif/arduino-esp32/blob/master/libraries/Preferences/src/Preferences.h
    //preferences.clear();
    //preferences.remove("VariableName");
    //Serial.println (preferences.freeEntries());
      preferences.begin(DeviceName);
      preferences.putString("MQTT_SERVER", mqtt_server.getValue());
      preferences.putInt("MQTT_PORT", atoi(mqtt_port.getValue()));
      preferences.putString("MQTT_ID", mqtt_id.getValue());
      preferences.putString("MQTT_USER", mqtt_user.getValue());
      preferences.putString("MQTT_PSWD", mqtt_pswd.getValue());
      preferences.putString("MQTT_TOPIC_OUT", mqtt_topic_out.getValue());
      preferences.putString("MQTT_TOPIC_IN", mqtt_topic_in.getValue());
      preferences.putString("MQTT_TOPIC_LWT", mqtt_topic_lwt.getValue());
      #ifdef FOTA_ENABLED 
      preferences.putString("FOTA_FILE", fota_file.getValue());
      #endif
      preferences.end();
      ReadPreferences();
      EcraseCredentialsMQTTduJSONlittleFS();
      Debug("Parameters saved !");
    }
#else
    preferences.begin(DeviceName);
    preferences.putString("MQTT_SERVER", MQTT_SERVER);
    preferences.putInt("MQTT_PORT", MQTT_PORT);
    preferences.putString("MQTT_ID", MQTT_ID);
    preferences.putString("MQTT_USER", MQTT_USER);
    preferences.putString("MQTT_PSWD", MQTT_PSWD);
    preferences.putString("MQTT_TOPIC_OUT", MQTT_TOPIC_OUT);
    preferences.putString("MQTT_TOPIC_IN", MQTT_TOPIC_IN);
    preferences.putString("MQTT_TOPIC_LWT", MQTT_TOPIC_LWT);
    #ifdef FOTA_ENABLED 
    preferences.putString("FOTA_FILE", FOTA_FILE);
    #endif
    preferences.end();
    ReadPreferences();
    EcraseCredentialsMQTTduJSONlittleFS();
    Debug("Parameters FORCED to FACTORY DEFAULT !!!");
#endif //FORCE_RESET_MQTT
  
  #ifdef FOTA_ENABLED
  Fota();
  #endif
}

void WiFiManagerPortal()
{
  Debug("Launch Portal to Reconfigure");
  for (int i=0; i<3; i++) { Debug("*", false); delay(1000); }
  /*
  preferences.begin(DeviceName);
  preferences.putBool("FORCE_PORTAL", true);
  preferences.end();
  */
  wifiManager.resetSettings();
  delay(1000);
  Debug(); Debug();
  ESP.restart();
}

void ConfigJSONlittleFS(String JSONconfig)
{
Debug("Force ConfigJSON =\r\n"+JSONconfig);
DynamicJsonDocument json(2500);
if (deserializeJson(json, JSONconfig) != DeserializationError::Ok)
{
  Debug(F("Error reading json"));
  return;
}
auto root = json.as<JsonObject>();
String msg;
RFLink::Config::pushNewConfiguration(root, msg, false);
//if (msg!="") Serial.println(msg);
RFLink::Config::saveConfigToFlash();
}

void EcraseCredentialsMQTTduJSONlittleFS()
{
  String JSONconfig;
  JSONconfig = "{'mqtt':{";
  JSONconfig+= "'enabled':true,";
  JSONconfig+= "'server':'"+MQTT_server+"',";
  JSONconfig+= "'port':"+String(MQTT_port)+",";
  JSONconfig+= "'id':'"+MQTT_id+"',";
  JSONconfig+= "'user':'"+MQTT_user+"',";
  JSONconfig+= "'password':'"+MQTT_pswd+"',";
  JSONconfig+= "'topic_out':'"+MQTT_topic_out+"',";
  JSONconfig+= "'topic_in':'"+MQTT_topic_in+"',";
  JSONconfig+= "'topic_lwt':'"+MQTT_topic_lwt+"',";
  JSONconfig+= "'lwt_enabled':true,";
  JSONconfig+= "'ssl_enabled':false,";
  JSONconfig+= "'ssl_insecure':true,";
  JSONconfig+= "'ca_cert':'',";
  JSONconfig+= "}}";
  ConfigJSONlittleFS(JSONconfig);
  // Autre methode = Config::executeCliCommand("10;config;set;{'mqtt':{'topic_in':'msg/RFLink/...'}}");
}

#endif // WIFIMANAGER_ENABLE


#ifdef FOTA_ENABLED
void Fota()
  {
  Debug();
  Debug("FOTA : "+FOTA_file, false);
  
  // Return if WiFi not connected
  unsigned long milli0=millis();
  while (millis()-milli0<10*1000 && WiFi.status() != WL_CONNECTED) { delay(500); Debug(".", false); } // Wait for WiFi connexion, maxi 10s
  Debug();
  if (WiFi.status() != WL_CONNECTED) return;

  // Read Reference of installed Firmware
  preferences.begin(DeviceName, true);
  String FirmWareCourant = preferences.getString("FirmWare");
  preferences.end();
  
  HTTPClient http;
  WiFiClient client;

  //Look for FirmWare Update
  String FirmWareDispo="";
  const char * headerKeys[] = {"Last-Modified"};
  const size_t numberOfHeaders = 1;
  http.begin(FOTA_file);
  http.collectHeaders(headerKeys, numberOfHeaders);
  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK)
    {
      Debug("FOTA : No file available (" + http.errorToString(httpCode) + ")");
      http.end();
      return;
    }
  FirmWareDispo=http.header((size_t)0);
  //int size = http.getSize();
  //if (size==-1) Serial.println("Taille inconnue");
  //else Serial.println("Taille Firmware ="+String(size));
  http.end();

  // Check Date of UpDate
  Debug("FOTA : Firware available = " + FirmWareDispo);
  if (FirmWareDispo=="" || FirmWareCourant==FirmWareDispo)
    {
    Debug("FOTA : no new UpDate !");
    return;
    }

  //Download process
  //httpUpdate.setLedPin(Led_Pin, LOW); // Value for LED ON
  t_httpUpdate_return ret;
  Debug();
  Debug("*********************");
  Debug("FOTA : DOWNLOADING...");
  httpUpdate.rebootOnUpdate(false);
  ret = httpUpdate.update(client, FOTA_file);
  switch (ret)
    {
    case HTTP_UPDATE_FAILED:
      Debug(String("FOTA : Uploading Error !") + httpUpdate.getLastError() + httpUpdate.getLastErrorString().c_str());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Debug("FOTA : UpDate not Available");
      break;
    case HTTP_UPDATE_OK:
      Debug("FOTA : Update OK !!!");
      Debug("*********************");
      Debug();
      preferences.begin(DeviceName);
      preferences.putString("FirmWare", FirmWareDispo);
      preferences.end();
      WiFi.persistent(true);
      delay(1000);
      ESP.restart();
      break;
    }
  }
#endif // FOTA_ENABLED

