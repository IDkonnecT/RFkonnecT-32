/*
Le Payload MQTT du message d'arrivée contient le code brut à envoyer au Thermostat
Si le thermostat répond, renvoie un MQTT avec un JSON contenant : ModeAuto,Boost,Window,Lock,Valve,Temp
ATTENTION :
- le scan répétitif met peut-être le beans
- risque de memory leakage car l'appel de NimBLEAddress(Mac) crée un objet
*/


#include <Arduino.h>
#include "RFLink.h"
#include "6_Credentials.h"

#ifdef EQ3THERMOSTAT_ENABLED

#include "_eQ3Thermostat.h"

//#define DEBUG

static std::map<BLERemoteCharacteristic*, NimBLEAddress> lookupMap;

void AddChar(char* s, char c)
{
  int len = strlen(s);
  s[len] = c;
  s[len+1] = 0;
}

void scanEndedCB(NimBLEScanResults results);

class ClientCallbacks : public NimBLEClientCallbacks
{
  void onConnect(NimBLEClient* pClient)
  {
    #ifdef DEBUG
    Serial.printf("eQ3 : %s => Connected OK\n", pClient->getPeerAddress().toString().c_str());
    #endif
    pClient->updateConnParams(120,120,0,60); /** Min interval: 120 * 1.25ms = 150, Max interval: 120 * 1.25ms = 150, 0 latency, 60 * 10ms = 600ms timeout */
  };

  void onDisconnect(NimBLEClient* pClient)
  {
    #ifdef DEBUG
    Serial.printf("eQ3 : %s => Disconnected !\n", pClient->getPeerAddress().toString().c_str());
    #endif
  };
      
  bool onConnParamsUpdateRequest(NimBLEClient* pClient, const ble_gap_upd_params* params)
  {/** When the peripheral requests a change to the connection parameters => true to accept */
    if(params->itvl_min < 24)                   return false;   /** 1.25ms units */
    else if(params->itvl_max > 40)              return false;   /** 1.25ms units */
    else if(params->latency > 2)                return false;   /** Number of intervals allowed to skip */
    else if(params->supervision_timeout > 100)  return false;    /** 10ms units */
    return true;
  };
};

static ClientCallbacks clientCB;    /** Create a single global instance of the callback class to be used by all clients */

bool isThermostat(NimBLEAdvertisedDevice* advertisedDevice)
{ return  advertisedDevice->haveServiceUUID() && advertisedDevice->isAdvertisingService(serviceUUID); }

class AdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks
{
  void onResult(NimBLEAdvertisedDevice* advertisedDevice)
  {
    //Serial.print("Device found: "); Serial.println(advertisedDevice->toString().c_str());
    if(isThermostat(advertisedDevice))
    {
      char RSSI [5];
      itoa(advertisedDevice->getRSSI(), RSSI, 10);
    //#ifdef DEBUG
      Serial.printf("eQ3 : %s visible => RSSI = %s\n", advertisedDevice->getAddress().toString().c_str(), RSSI);
    //#endif
      Publier(advertisedDevice->getAddress().toString().c_str(), RSSI);
    }
  };
};

void notifyCB(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify)
{
#ifdef DEBUG
  Serial.printf("eQ3 : %s => Notif", lookupMap[pRemoteCharacteristic].toString().c_str());
  Serial.print(" DEC= ");
    for (int i = 0; i < length; i++) { Serial.print((byte)pData[i], DEC); Serial.print(' ');}
  Serial.print("/ HEX= ");
    for (int i = 0; i < length; i++) { Serial.print((byte)pData[i], HEX); Serial.print(' ');}
  Serial.println();
#endif

  if (!(pData[0] == 0x02 && pData[1] == 0x01))
  {
  #ifdef DEBUG
      Serial.printf("eQ3 : %s => Program Notif !\n", lookupMap[pRemoteCharacteristic].toString().c_str());
  #endif
      return;
  }
  
  char lock = '?';
  char window = '?';
  char mode = '?';
  char boost = '?';
  char stateChr = 0;
  stateChr = pData[2] & 0xf0;
  switch (stateChr)
  {
      case 0x00: lock = '0';  window = '0';  break;
      case 0x01: lock = '1';  window = '1';  break;
      case 0x02: lock = '1';  window = '0';  break;
      case 0x03: lock = '1';  window = '1';  break;
      default:   lock = '?';  window = '?';
  }
  stateChr = pData[2] & 0x0f;
  switch (stateChr)
  {
      case 0x08: mode = 'A';  boost = '0';  break;
      case 0x09: mode = 'M';  boost = '0';  break;
      case 0x0A: mode = 'H';  boost = '0';  break;
      case 0x0C: mode = 'A';  boost = '1';  break;
      case 0x0D: mode = 'M';  boost = '1';  break;
      case 0x0E: mode = 'H';  boost = '1';  break;
      default:   mode = '?';  boost = '?';
  }

  int valve = (((int)pData[3]) * 100) / 0x64;
  int temp = ((int)pData[5] * 10) / 2;

  char JsonTh[64];
  char Buffer[4];
  /*
  strcpy(JsonTh, "{'Mode':'"); AddChar(JsonTh, mode);
  strcat(JsonTh, "','Boost':'"); AddChar(JsonTh, boost);
  strcat(JsonTh, "','Win':'"); AddChar(JsonTh, window);
  strcat(JsonTh, "','Lock':'"); AddChar(JsonTh, lock);
  itoa(valve, Buffer, 10); strcat(JsonTh,"','V':"); strcat(JsonTh, Buffer);
  itoa(temp, Buffer, 10);  strcat(JsonTh,",'T':"); strcat(JsonTh, Buffer);
  strcat(JsonTh, "}");
  */
  strcpy(JsonTh, "{\"Mode\":\""); AddChar(JsonTh, mode);
  strcat(JsonTh, "\",\"Boost\":\""); AddChar(JsonTh, boost);
  strcat(JsonTh, "\",\"Win\":\""); AddChar(JsonTh, window);
  strcat(JsonTh, "\",\"Lock\":\""); AddChar(JsonTh, lock);
  itoa(valve, Buffer, 10); strcat(JsonTh,"\",\"V\":"); strcat(JsonTh, Buffer);
  itoa(temp, Buffer, 10);  strcat(JsonTh,",\"T\":"); strcat(JsonTh, Buffer);
  strcat(JsonTh, "}");

  Publier(lookupMap[pRemoteCharacteristic].toString().c_str(), JsonTh);
//#ifdef DEBUG
  Serial.printf("eQ3 : %s => %s\n", lookupMap[pRemoteCharacteristic].toString().c_str(), JsonTh);
//#endif
}

void scanEndedCB(NimBLEScanResults results)
{
#ifdef DEBUG
  Serial.println("Scan Ended");
#endif
}

bool connectThermo(NimBLEAddress Device, byte *payload, unsigned int length)
{
  NimBLEDevice::getScan()->stop();
  NimBLEClient* pClient = nullptr;
  
  if(NimBLEDevice::getClientListSize()) /** Check if we have a client we should reuse first */
  {/** If device known, send false in connect() to prevent refreshing the service database */
    pClient = NimBLEDevice::getClientByPeerAddress(Device);
    if(pClient)
    {
      if(pClient->isConnected())
      {
      #ifdef DEBUG
        Serial.printf("eQ3 : %s => Already connected\n", pClient->getPeerAddress().toString().c_str());
      #endif
      }
      else
      {
      #ifdef DEBUG
        Serial.printf("eQ3 : %s => Reactivate connexion %s\n", Device.toString().c_str(), pClient->getPeerAddress().toString().c_str());
      #endif
        //pClient->setConnectTimeout(10);
        if(!pClient->connect(Device, false))
        {
        #ifdef DEBUG
          Serial.printf("eQ3 : %s => Reactivate failed !\n", Device.toString().c_str());
        #endif
          return false;
        }
      #ifdef DEBUG
        Serial.printf("eQ3 : %s => Reactivate OK\n", Device.toString().c_str());
      #endif
      }
    } 
    /** If no client knows this device, check for a client that is disconnected that we can use. */
    else
    {
      pClient = NimBLEDevice::getDisconnectedClient();
    #ifdef DEBUG
      Serial.printf("eQ3 : %s => ReUse disconnected Client\n", Device.toString().c_str());
      // If call here of pClient->getPeerAddress().toString().c_str()) => generates a Crash !
    #endif
    }
  }
  
  if(!pClient) /** No client to reuse => create a new one. */
  {
    if(NimBLEDevice::getClientListSize() >= NIMBLE_MAX_CONNECTIONS)
    {
    #ifdef DEBUG
      Serial.printf("eQ3 : %s =>Max BLE clients reached !\n", Device.toString().c_str());
    #endif
      return false;
    }
    pClient = NimBLEDevice::createClient();
  #ifdef DEBUG
    Serial.printf("eQ3 : %s => New client created\n", pClient->getPeerAddress().toString().c_str());
  #endif
    pClient->setClientCallbacks(&clientCB, false);
    pClient->setConnectionParams(12,12,0,51); /** Min interval: 12 * 1.25ms = 15, Max interval: 12 * 1.25ms = 15, 0 latency, 51 * 10ms = 510ms timeout */
    pClient->setConnectTimeout(10); /** Set how long we are willing to wait for the connection to complete (seconds), default is 30. */
    
    if (!pClient->connect(Device))
    {/** Created a client but failed to connect, don't need to keep it as it has no data */
    #ifdef DEBUG
      Serial.printf("eQ3 : %s => Failed to connect, client deleted\n", Device.toString().c_str());
    #endif
      NimBLEDevice::deleteClient(pClient);
      return false;
    }
  }       
  
  if(!pClient->isConnected())
  {
    //pClient->setConnectTimeout(10);
    if (!pClient->connect(Device))
    {
    #ifdef DEBUG
      Serial.printf("eQ3 : %s => Failed to connect\n", Device.toString().c_str());
    #endif
      return false;
    }
  }
  
#ifdef DEBUG
  Serial.printf("eQ3 : %s => Connected - RSSI = %i\n", pClient->getPeerAddress().toString().c_str(), pClient->getRssi());
#endif
  
  /** Now we can read/write/subscribe the charateristics of the services we are interested in */
  NimBLERemoteService* pSvc = nullptr;
  NimBLERemoteCharacteristic* pChr = nullptr;
  
  pSvc = pClient->getService(serviceUUID);
  // Thermostat Notify Charateristic check and subscribe
  pChr = nullptr;
  if(pSvc) { pChr = pSvc->getCharacteristic(notifyUUID); }
#ifdef DEBUG
  else { Serial.printf("eQ3 : %s => Service not found\n", pClient->getPeerAddress().toString().c_str()); }
#endif
  if(pChr) 
  {     
    if(pChr->canNotify())
    {
      if(!pChr->subscribe(true, notifyCB)) { pClient->disconnect(); return false; }
      lookupMap[pChr] = Device.toString();
    #ifdef DEBUG
      Serial.printf("eQ3 : %s => Notification set\n", pClient->getPeerAddress().toString().c_str());
    #endif
    }
  }
#ifdef DEBUG
  else { Serial.printf("eQ3 : %s => Notification not Found !\n", pClient->getPeerAddress().toString().c_str()); }
#endif
  // Thermostat Command Charateristic check
  pChr = nullptr;
  if(pSvc) { pChr = pSvc->getCharacteristic(commandUUID); }
  if(pChr)
  {
  #ifdef DEBUG
    if(pChr->canRead()) { Serial.printf("eQ3 : %s => Command Readable\n", pClient->getPeerAddress().toString().c_str()); }
  #endif
    if(pChr->canWrite())
    {
    #ifdef DEBUG
      Serial.printf("eQ3 : %s => Command Writeable\n", pClient->getPeerAddress().toString().c_str());
    #endif
      if (pChr->writeValue(payload, length, true))
      {
      #ifdef DEBUG
        Serial.printf("eQ3 : %s => CMD", pClient->getPeerAddress().toString().c_str());
        Serial.print(" DEC= ");
          for (int i = 0; i < length; i++) { Serial.print((byte)payload[i], DEC); Serial.print(' ');}
        Serial.print("/ HEX= ");
          for (int i = 0; i < length; i++) { Serial.print((byte)payload[i], HEX); Serial.print(' ');}
        Serial.println();
      #endif
      }
    }
  }
#ifdef DEBUG
  else { Serial.printf("eQ3 : %s => Command not Found !\n", pClient->getPeerAddress().toString().c_str()); }
  Serial.printf("eQ3 : %s => Done !!!\n", Device.toString().c_str());
#endif
  return true;
}


/////////////////////////////////////
// EQ3Thermostat high level functions

void SetupThermostats()
{
#ifdef DEBUG
  Serial.println("Starting NimBLE Client");
#endif
  NimBLEDevice::init(DeviceName);
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);
}

void ScanThermostats(unsigned int DureeScan)
{
  static unsigned long LastScanThermostats = 0;
  if (LastScanThermostats!=0 && millis()-LastScanThermostats<2*DureeScan*1000) return;
  LastScanThermostats=millis();
#ifdef DEBUG
  Serial.print("eQ3 scanning : "); Serial.print(millis()/60000); Serial.println("min");
#endif
  NimBLEScan* pScan = NimBLEDevice::getScan(); 
  pScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
  pScan->setInterval(45); /** Set scan interval (how often) and window (how long) in milliseconds */
  pScan->setWindow(15);
  pScan->setActiveScan(true);
  pScan->start(DureeScan, scanEndedCB);
}


/////////////////////
// RFLink integration

#include <PubSubClient.h>
#include "6_MQTT.h"

extern String MQTT_topic_out;
extern String MQTT_topic_in;

bool eQ3MQTT(char *topic, byte *payload, unsigned int length)
{
  // Quit if topic does not contain Themrostat Address
  if (strcmp(topic, MQTT_topic_in.c_str())==0) return false;
  // Extract Thermostat Number after last '/'
#ifdef DEBUG
  Serial.println();
  memFree();
#endif
  Serial.print("eQ3 MSG : ");
  char* NoSerie=nullptr;
  for (int i=0; i<strlen(topic); i++)
    if (topic[i]=='/') NoSerie=&topic[i+1];
  Serial.println(NoSerie);
  if (strlen(NoSerie)!=12) return false;
  // Reconstruct Mac Address by adding ':' every 2 character
  static char Mac[18];
  int i, j = 0;
  for (i = 0; i<12; i++)
    {
    Mac[j++]=NoSerie[i];
    if (i%2==1 && i!=11) Mac[j++]=':';
    }
  Mac[j++] = '\0';
  //SetTemp
  connectThermo(NimBLEAddress(Mac), payload, length);
  return true;
}

void Publier(const char* Thermo, const char* Message)
{
  static char NoSerie[16]="";
  int i, j = 0;
  for (i = 0; Thermo[i]!='\0'; i++)             // 'i' moves through all of original 's'
    if (Thermo[i]!=':') NoSerie[j++]=Thermo[i]; // 'j' only moves after we write a non-'chr'
  NoSerie[j] = '\0';

  String Topic=MQTT_topic_out+"/eQ3/"+NoSerie;
  RFLink::Mqtt::Publie(Topic.c_str(), Message, false);
}

void memFree()
{
#ifdef DEBUG
  Serial.printf("*** ESP freeHeap = %i ***\n", ESP.getFreeHeap());
#endif
}

#endif