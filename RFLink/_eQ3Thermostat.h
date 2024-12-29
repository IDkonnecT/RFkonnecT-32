/*
Protocol Hacking :
  https://reverse-engineering-ble-devices.readthedocs.io/en/latest/protocol_description/00_protocol_description.html
Passerelle mqtt :
  https://github.com/softypit/esp32_mqtt_eq3
Librarie :
  https://platformio.org/lib/show/11070/NimBLE-Arduino/examples
  https://platformio.org/lib/show/6756/ESP-EQ3/examples => Does not work with more than 2 Thermo
*/

#ifndef _eQ3Thermostat_h
#define _eQ3Thermostat_h

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <map>

static NimBLEUUID serviceUUID = NimBLEUUID("3e135142-654f-9090-134a-a6ff5bb77046");
static NimBLEUUID notifyUUID = NimBLEUUID("d0e8434d-cd29-0996-af41-6c90f4e0eb2a");
static NimBLEUUID commandUUID = NimBLEUUID("3fa4585a-ce4a-3bad-db4b-b8df8179ea09");

/////////////////////////////////////
// EQ3Thermostat high level functions

void SetupThermostats();
void ScanThermostats(unsigned int DureeScan);

/////////////////////
// RFLink integration

bool eQ3MQTT(char *topic, byte *payload, unsigned int length);
void Publier(const char* Thermo, const char* Message);
void memFree();

#endif