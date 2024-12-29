//#######################################################################################################
//##                    This Plugin is only for use with the RFLink software package                   ##
//##                                 Plugin-101: FineOffset WIND Station protocol                                   ##
//#######################################################################################################
/*******************************************************************************************************\
 * This plugin takes care of receiving FineOffset WIND Station protocol. 
 * 
 * Author  (present)  : IDkonnecT - Thierry PERRON
 * Support (present)  : 
 * Author  (original) : Tommy Vestermark / Christian W. Zuckschwerdt <zany@triq.net>
 * Support (original) : https://github.com/merbanan/rtl_433/blob/26a82455775b15389e4b5a134c6c87c76a55f46e/src/devices/fineoffset.c
 * License            : This code is free for use in any open source project when this header is included.
 *                      Usage of any parts of this code in a commercial application is prohibited!
 ********************************************************************************************************
 * Technical information:
 * Fine Offset Electronics WS0232 Wind/temperature/Humidity sensor protocol (Rain option).
 * 
 * /!\ IMPORTANT /!\
 * => Frequency = 433,82Mhz !!!
 * => Mandatory to set in 2_Signal.h : SIGNAL_END_TIMEOUT_US 3000
 * (because delay betwen messages is only 3400µs)
 * 
 * Modulation = OOK with PWM
 * Long = 1000 us, short = 532 us, gap = 484 us.
 * Very long and very short pulses are for Preamble (see under)
 *  
 * Data layout:
 * P XX II II ?? DT TT HH WW GG ?? ?? ?? SS CC
 * - P: PREMABLE with init SHORT pulse + LONG gap + SHORT pulse + LONG pulse + gap
 * - X: 8 bit Sensor ID
 * - I: 16 bit ID
 * - D: 4 bit wind Direction (0=North)
 * - T: 12 bit Temperature, with first bit is sign
 * - H: 8 bit Himidity
 * - W: 8 bit average Wind Speed (0.43m/s)
 * - G: 8 bit Wind Gust Speed (0.43m/s)
 * - S: 8 bit checkSum
 * - W: 8 bit CRC-8 with poly 0x31 init 0x00
 *  
 \******************************************************************************************************/
//#define DEBUG_WS3032

#define FINEOFFSET_WS3032_PLUGIN_ID 102
#define PLUGIN_DESC_102 "FineOffset"

#define FINEOFFSET_WS3032_BIT 112
#define FINEOFFSET_WS3032_BYTE 14+1

#define FINEOFFSET_WS3032_PULSESYNCl_D 1300
#define FINEOFFSET_WS3032_PULSE01_D 600
#define FINEOFFSET_WS3032_PULSESPACE_D 484
#define FINEOFFSET_WS3032_PULSETOL_D 100
#ifdef DEBUG_WS3032
   #define FINEOFFSET_WS3032_PULSESYNCs_D 200
#endif

#ifdef PLUGIN_102
#include "../4_Display.h"
#include "../7_Utils.h"

#ifdef DEBUG_WS3032
void PrintPulses(int Debut, int Fin);
void FiltreSignal(int Debut, int Fin);
#endif 

int CheckPreamble(const long FINEOFFSET_WS3032_PULSESYNCl, const long FINEOFFSET_WS3032_PULSESPACE, const long FINEOFFSET_WS3032_PULSETOL)
{
   int i;
   for (i=0; i<5; i++)
      if (( RawSignal.Pulses[i+0]>FINEOFFSET_WS3032_PULSESYNCl
         && RawSignal.Pulses[i+1]<FINEOFFSET_WS3032_PULSESPACE
         && RawSignal.Pulses[i+2]>FINEOFFSET_WS3032_PULSESYNCl
         && RawSignal.Pulses[i+3]>FINEOFFSET_WS3032_PULSESYNCl
         && RawSignal.Pulses[i+4]<FINEOFFSET_WS3032_PULSESPACE+FINEOFFSET_WS3032_PULSETOL ))
         return i+5;
   return -1;
}

boolean Plugin_102(byte function, const char *string)
{
   byte Message[FINEOFFSET_WS3032_BYTE]={0};
   // Check length
#ifndef DEBUG_WS3032
   if (RawSignal.Number!=FINEOFFSET_WS3032_BIT*2+4) return false;
#else
   if (RawSignal.Number>160) Serial.printf("**********\nEntree longue : %i\n", RawSignal.Number); else return false;
   if (RawSignal.Number!=FINEOFFSET_WS3032_BIT*2+4) Serial.printf("Longueur incorrecte != %i\n", FINEOFFSET_WS3032_BIT*2+5);
#endif

   const long FINEOFFSET_WS3032_PULSESYNCl = FINEOFFSET_WS3032_PULSESYNCl_D / RawSignal.Multiply;
   const long FINEOFFSET_WS3032_PULSE01 =    FINEOFFSET_WS3032_PULSE01_D / RawSignal.Multiply;
   const long FINEOFFSET_WS3032_PULSESPACE = FINEOFFSET_WS3032_PULSESPACE_D / RawSignal.Multiply;
   const long FINEOFFSET_WS3032_PULSETOL =   FINEOFFSET_WS3032_PULSETOL_D / RawSignal.Multiply;
#ifdef DEBUG_WS3032
   const long FINEOFFSET_WS3032_PULSESYNCs = FINEOFFSET_WS3032_PULSESYNCs_D / RawSignal.Multiply;
#endif

   int i0=CheckPreamble(FINEOFFSET_WS3032_PULSESYNCl, FINEOFFSET_WS3032_PULSESPACE, FINEOFFSET_WS3032_PULSETOL);
   if (i0==-1)
   {
   #ifndef DEBUG_WS3032
      return false;
   #else
      Serial.println("/!\\ Mauvais Preamble !!!");
   #endif
   }
#ifdef DEBUG_WS3032
   else { Serial.printf("i0 = %i\n", i0); }
   //PrintPulses(0, RawSignal.Number);
   //FiltreSignal(i0, RawSignal.Number, FINEOFFSET_WS3032_PULSESYNCs);
   //Serial.println("Signal filtré : ");
   PrintPulses(i0, RawSignal.Number);
#endif
   if (RawSignal.Number-i0!=FINEOFFSET_WS3032_BIT*2-1)
   {
   #ifndef DEBUG_WS3032
      return false;
   #else
      Serial.println("/!\\ Nb bit incorrect !!!");
   #endif
   }
   
   // Bistream construction
   int i, j=0;
   for (i=i0; i<RawSignal.Number; i=i+2)
   {
      Message[j] <<= 1;
      if (RawSignal.Pulses[i]>FINEOFFSET_WS3032_PULSE01) Message[j] |= 0x1;
      if (i+1<RawSignal.Number && abs(RawSignal.Pulses[i+1]-FINEOFFSET_WS3032_PULSESPACE)>FINEOFFSET_WS3032_PULSETOL)
         {
         #ifndef DEBUG_WS3032
            return false;
         #else
            Serial.print("GAP NOK ! ");
         #endif
         }
      if ((i-i0)%16==14) j++;
   }
   #ifdef DEBUG_WS3032
   Serial.println(" => Bytes encodés :");
   printBytes(Message, FINEOFFSET_WS3032_BYTE);
   #endif

   // Check Device Type
   if (Message[0]!=0xf5)
   {
   #ifndef DEBUG_WS3032
      return false;
   #else
      Serial.println("/!\\ Erreur Type");
   #endif
   }
   // CheckSum
   if ( (add_bytes(Message,12) & 0xff) != Message[12])
   {
   #ifndef DEBUG_WS3032
      return false;
   #else
      Serial.println("/!\\ Erreur ChechSum");
   #endif
   }
   // CRC
   if (crc8(Message, 13, 0x31, 0) != Message[13])
   {
   #ifndef DEBUG_WS3032
      return false;
   #else
      Serial.println("/!\\ Erreur CRC");
   #endif
   }
   // Data
   int id         = (Message[1] << 8) | Message[2];
   int bat        = !(Message[3] & 0x80);
   int temp_sign  = (Message[4] & 0x08) ? -1 : 1;
   int temp_raw   = ((Message[4] & 0x07) << 8) | Message[5];
   int temp       = temp_sign * temp_raw;
   int hum        = Message[6];
   int dir        = Message[4] >> 4;
   int speed      = (Message[7]*1548)/100;   // *0.43f*36f m/s -> 0.1km/h
   int gust       = (Message[8]*1548)/100;   // *0.43f*36f m/s -> 0.1km/h
   //int rain_raw = (Message[9] << 16) | (Message[10] << 8) | Message[11]; // /!\ Not Sure !

#ifdef DEBUG_WS3032
   Serial.printf("id=%i - ", id);
   Serial.printf("bat=%i - ", bat);
   Serial.printf("temp=%i - ", temp);
   Serial.printf("hum=%i - ", hum);
   Serial.printf("dir(°)=%i - ", dir*225/10);
   Serial.printf("speed(0.1k/h)=%i - ", speed);
   Serial.printf("gust(0.1k/h)=%i\n", gust);
#endif
   //==================================================================================
   // Output
   //==================================================================================
#ifndef DEBUG_WS3032
   display_Header();
   display_Name(PSTR(PLUGIN_DESC_102));
   display_IDn(id, 2);
   display_TEMP(temp);        // => HEX 0.1°C
   display_HUM(hum);          // => DEC 1%
   display_WINDIR(dir);       // => 0=N/14=NW (15=N-NW)
   display_WINSP(speed);      // => HEX 0.1km/h
   display_WINGS(gust);       // => HEX 0.1km/h
   display_BAT(bat);
   display_Footer();
 #endif
   //==================================================================================
   RawSignal.Repeats = true; // suppress repeats of the same RF packet
   RawSignal.Number = 0;
   return true;
}


#ifdef DEBUG_WS3032
void PrintPulses(int Debut, int Fin)
{
   Serial.printf("Pulses = %i\n", Fin-Debut);

   for (int i=Debut; i<Fin; i=i+1)
   {
      Serial.printf("%i ", RawSignal.Pulses[i]*RAWSIGNAL_SAMPLE_RATE);
      if ((i-Debut)%2==1) Serial.print("  |   ");
      if ((i-Debut)%16==15) Serial.println();
   }
   Serial.println();

   for (int i=Debut; i<Fin; i=i+1)
   {
      if ((i-Debut)%2==0)
         { if (RawSignal.Pulses[i]>FINEOFFSET_WS3032_PULSE01) Serial.print('1'); else Serial.print('0'); }
      if ((i-Debut)%16==14) Serial.print(' ');
   }
   Serial.println();
}

void FiltreSignal(int Debut, int Fin, FINEOFFSET_WS3032_PULSESYNCs)
{// Suppress pulses shorter than FINEOFFSET_WS3032_PULSESYNCs
   int j=Debut;
   for (int i=Debut; i<Fin; i++)
      if (RawSignal.Pulses[i]>=FINEOFFSET_WS3032_PULSESYNCs)
         RawSignal.Pulses[j++]=RawSignal.Pulses[i];
      else Serial.printf("%i->%i / ", i-Debut, RawSignal.Pulses[i]*RAWSIGNAL_SAMPLE_RATE);
   RawSignal.Number=j;
   Serial.println();
}
#endif
/*
void printBytes(byte *Message, int nBytes)
{
   for (int Byte=0; Byte<nBytes; Byte++)
   {
      for (int Bit=7; Bit>=0; Bit--)
         Serial.print((Message[Byte] >> Bit) & 0x1);
      Serial.print(' ');
   }
   Serial.println();
}
*/

#endif // PLUGIN_102