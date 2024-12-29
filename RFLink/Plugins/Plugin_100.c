//#######################################################################################################
//##                    This Plugin is only for use with the RFLink software package                   ##
//##                                 Plugin-100: Harvesting Power Switch                               ##
//#######################################################################################################
/*******************************************************************************************************\
 * This plugin takes care of receiving Harvesting Power Switch protocol. 
 * 
 * Author  (present)  : IDkonnecT - Thierry PERRON
 * Support (present)  : 
 * License            : This code is free for use in any open source project when this header is included.
 *                      Usage of any parts of this code in a commercial application is prohibited!
 ********************************************************************************************************
 * Technical information: no battery RF433 switches found on AliExpress
 * SAFUL :  https://fr.aliexpress.com/item/4001286646052.html?spm=a2g0s.9042311.0.0.27426c37slQEbZ
 * ZOGIN :  https://fr.aliexpress.com/item/4000180776353.html?spm=a2g0s.9042311.0.0.27426c37slQEbZ
 * CACAZI : https://fr.aliexpress.com/item/4000238439346.html?spm=a2g0s.9042311.0.0.27426c37slQEbZ
 * and much more...
 *  
 * Pulses :
 * 1 2 3... 50 51 52 P+50
 * S P L/C     C  P
 * 
 * /!\ RFM69OOK.cpp necessay configuration : RFM69OOK.ccp => bitrate: 19200 kbps (8*µs/bit)
 * { REG_BITRATEMSB, RF_BITRATEMSB_19200 },
 * { REG_BITRATELSB, RF_BITRATELSB_19200 },
 * { REG_FRFMSB, 0x6C }, // Rem : FStep = FXOsc / 2^19
 * { REG_FRFMID, 0x7A }, //       FRF = FStep * Frf
 * { REG_FRFLSB, 0xE1 }, //       FRF = 433.92 MHz
 * 
 * /!\ 2_Signal.h necessay configuration :
 * #define RAWSIGNAL_SAMPLE_RATE 16    // ! instead of 32
 * #define MIN_PULSE_LENGTH_US   10    // ! instead of 100 or 250
 * 
 \******************************************************************************************************/
#define HARVEST_PLUGIN_ID 100
#define PLUGIN_DESC_100 "HARVEST"

#define HARVEST_PULSECOUNT 50

#define HARVEST_PULSEMID_D 50
#define HARVEST_PULSEMAX_D 150
#define HARVEST_PAUSEMIN_D 800

#ifdef PLUGIN_100
#include "../4_Display.h"

boolean Plugin_100(byte function, const char *string)
{
   // Long Pauses after Preamble Pulse and between messages
   if (RawSignal.Number<HARVEST_PULSECOUNT*3) return false;

   const long HARVEST_PULSEMID = HARVEST_PULSEMID_D / RawSignal.Multiply;
   const long HARVEST_PULSEMAX = HARVEST_PULSEMAX_D / RawSignal.Multiply;
   const long HARVEST_PAUSEMIN = HARVEST_PAUSEMIN_D / RawSignal.Multiply;

   if (RawSignal.Pulses[0]<HARVEST_PAUSEMIN) return false;
   if (RawSignal.Pulses[1]>HARVEST_PULSEMAX) return false;

   // Search Correct subMessages
   unsigned long bitstream = 0L;
   int PauseIndex[10]={0};
   int NbCorrectMsg=0;
   int Start=0;
   bool SoFarSoGoog=true;
   for (int i=0; i<=RawSignal.Number; i=i+1)
   {
      if (RawSignal.Pulses[i]>HARVEST_PAUSEMIN)
         {
            if (SoFarSoGoog && i-Start==HARVEST_PULSECOUNT) PauseIndex[NbCorrectMsg++]=Start;
            Start=i;
            SoFarSoGoog=true;
         }
      //else if (RawSignal.Pulses[i]>HARVEST_PULSEMAX) SoFarSoGoog=false;
      else if ( (i-Start)%2==0 && (RawSignal.Pulses[i]+RawSignal.Pulses[i-1]>HARVEST_PULSEMAX) ) SoFarSoGoog=false;
      if (NbCorrectMsg>=10) break;
   }
   if (NbCorrectMsg<2) return false;
   // Test that 2 first goog messages are consistant and equal
   for (int i=0; i<HARVEST_PULSECOUNT; i=i+2)
      {
      if (RawSignal.Pulses[i+PauseIndex[1]]<HARVEST_PULSEMID && RawSignal.Pulses[i+PauseIndex[2]]>HARVEST_PULSEMID) return false;
      if (RawSignal.Pulses[i+PauseIndex[1]]>HARVEST_PULSEMID && RawSignal.Pulses[i+PauseIndex[2]]<HARVEST_PULSEMID) return false;
      }
   // Output result
   for (int i=1; i<HARVEST_PULSECOUNT-2; i=i+2)
      {
      bitstream <<= 1;
      if (RawSignal.Pulses[i+PauseIndex[1]]>HARVEST_PULSEMID) bitstream |= 0x1;
      }
   //==================================================================================
   // Output
   //==================================================================================
   display_Header();
   display_Name(PSTR(PLUGIN_DESC_100));
   display_IDn(bitstream, 6);
   display_Footer();
   //==================================================================================
   RawSignal.Repeats = true; // suppress repeats of the same RF packet
   RawSignal.Number = 0;
   return true;
}

#endif // PLUGIN_100
