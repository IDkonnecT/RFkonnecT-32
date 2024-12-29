//#######################################################################################################
//##                    This Plugin is only for use with the RFLink software package                   ##
//##                                 Plugin-101: FineOffset RAIN Station protocol                                   ##
//#######################################################################################################
/*******************************************************************************************************\
 * This plugin takes care of receiving FineOffset Rain Station protocol. 
 * 
 * Author  (present)  : IDkonnecT - Thierry PERRON
 * Support (present)  : 
 * License            : This code is free for use in any open source project when this header is included.
 *                      Usage of any parts of this code in a commercial application is prohibited!
 ********************************************************************************************************
 * Technical information:
 * Fine Offset Electronics WH0530 Temperature/Rain sensor protocol,
 * also Agimex Rosenborg 35926 (sold in Denmark).
 * 
 * The sensor sends two identical packages of 71 bits each ~48s. The bits are PWM modulated with On Off Keying.
 * Data consists of 7 bit preamble and 8 bytes.
 * 
 * Data layout:
 *     38 a2 8f 02 00 ff e7 51
 *     FI IT TT RR RR ?? CC AA
 * 
 * - F: 4 bit fixed message type (0x3)
 * - I: 8 bit Sensor ID (guess). Does not change at battery change.
 * - B: 1 bit low battery indicator
 * - T: 11 bit Temperature (+40*10) (Upper bit is Battery Low indicator)
 * - R: 16 bit (little endian) rain count in 0.3 mm steps, absolute with wrap around at 65536
 * - ?: 8 bit Always 0xFF (maybe reserved for humidity?)
 * - C: 8 bit CRC-8 with poly 0x31 init 0x00
 * - A: 8 bit Checksum of previous 7 bytes (addition truncated to 8 bit)
 * 
 \******************************************************************************************************/
#define FINEOFFSET_WH0530_PLUGIN_ID 101
#define PLUGIN_DESC_101 "FineOffset"

#define FINEOFFSET_PULSECOUNT 142

#define FINEOFFSET_PULSEZERO_D 1440
#define FINEOFFSET_PULSEONE_D 470
#define FINEOFFSET_PULSESPACE_D 970
#define FINEOFFSET_PULSETOL_D 100

#ifdef PLUGIN_101
#include "../4_Display.h"
#include "../7_Utils.h"

void printBytes(byte *Message, int nBytes);
void printByte(byte Byte);
byte subBit(byte Byte, byte debut, byte longueur);

boolean Plugin_101(byte function, const char *string)
{
   // Check length
   if (RawSignal.Number!=FINEOFFSET_PULSECOUNT) return false;

   const long FINEOFFSET_PULSEZERO =   FINEOFFSET_PULSEZERO_D / RawSignal.Multiply;
   const long FINEOFFSET_PULSEONE =    FINEOFFSET_PULSEONE_D / RawSignal.Multiply;
   const long FINEOFFSET_PULSESPACE =  FINEOFFSET_PULSESPACE_D / RawSignal.Multiply;
   const long FINEOFFSET_PULSETOL =    FINEOFFSET_PULSETOL_D / RawSignal.Multiply;

   // Process signal
   byte Message[8];
   byte Byte = 0;
   byte bitStream = 0;
   #define PREAMBLE_PULSES 2*7
   for (int i=1; i<FINEOFFSET_PULSECOUNT; i=i+2)
   {
      // Check first 7 bits = 1
      if (i<=PREAMBLE_PULSES && abs(RawSignal.Pulses[i]-FINEOFFSET_PULSEONE)>FINEOFFSET_PULSETOL) return false;
      // Process follwings bits
      if (i>PREAMBLE_PULSES) bitStream <<= 1;
      if (abs(RawSignal.Pulses[i]-FINEOFFSET_PULSEONE)<FINEOFFSET_PULSETOL) bitStream |= 0x1;
      else if (abs(RawSignal.Pulses[i]-FINEOFFSET_PULSEZERO)<FINEOFFSET_PULSETOL) bitStream |= 0x0;
      else return false;
      if (i+1<FINEOFFSET_PULSECOUNT && abs(RawSignal.Pulses[i+1]-FINEOFFSET_PULSESPACE)>FINEOFFSET_PULSETOL) return false;
      // Process Byte
      if (i>PREAMBLE_PULSES && (i+1-PREAMBLE_PULSES)%16==0) { Message[Byte++] = bitStream; bitStream=0; }
   }
   // Check Device Type
   if (subBit(Message[0], 1, 4)!=0x03) return false;
   // CRC
   if (crc8(Message, 6, 0x31, 0) != Message[6]) return false;
   // CheckSum
   if ( (add_bytes(Message,7) & 0xff) != Message[7]) return false;
   // Data
    int id    = ((Message[0] & 0x0f) << 4) | (Message[1] >> 4);
    int bat   = (Message[1] >> 3) & 0x1;
    int temp  = ((Message[1] & 0x7) << 8 | Message[2]) - 400;
    int rain  = (Message[4] << 8 | Message[3]) *3;
   //==================================================================================
   // Output
   //==================================================================================
   display_Header();
   display_Name(PSTR(PLUGIN_DESC_101));
   display_IDn(id, 2);
   display_TEMP(temp);
   display_RAIN(rain);
   display_BAT(bat);
   display_Footer();
   //==================================================================================
   RawSignal.Repeats = true; // suppress repeats of the same RF packet
   RawSignal.Number = 0;
   return true;
}

byte subBit(byte Byte, byte debut, byte longueur) // Début = 1...
{
   Byte = Byte << (debut-1);
   Byte = Byte >> (8-longueur);
   return Byte;
}

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

void printByte(byte Byte)
{
   for (int Bit=7; Bit>=0; Bit--)
         Serial.print((Byte >> Bit) & 0x1);
   Serial.println();
}

/*
void printBitStream(unsigned long long int bitstream);
unsigned long int subBitStream64(unsigned long long int bitstream, byte debut, byte longueur);
byte checkSum(unsigned long long int bitstream, byte NbOctets);
byte crc8(unsigned long long int bitstream, unsigned nBytes, uint8_t polynomial, uint8_t init);

boolean Plugin_101(byte function, char *string)
{
   // Check length
   if (RawSignal.Number!=FINEOFFSET_PULSECOUNT) return false;
   // Process signal
   unsigned long long bitstream = 0L;
   for (int i=1; i<FINEOFFSET_PULSECOUNT; i=i+2)
   {
      // Check first 7 bits = 1
      if (i<=7 && abs(RawSignal.Pulses[i]-FINEOFFSET_PULSEONE)>FINEOFFSET_PULSETOL) return false;
      // Process follwings bits
      if (i>7) bitstream <<= 1;
      if (abs(RawSignal.Pulses[i]-FINEOFFSET_PULSEONE)<FINEOFFSET_PULSETOL) bitstream |= 0x1;
      else if (abs(RawSignal.Pulses[i]-FINEOFFSET_PULSEZERO)<FINEOFFSET_PULSETOL) bitstream |= 0x0;
      else return false;
      if (i+1!=FINEOFFSET_PULSECOUNT && abs(RawSignal.Pulses[i+1]-FINEOFFSET_PULSESPACE)>FINEOFFSET_PULSETOL) return false;
   }
   // Check Device Type
   if (subBitStream64(bitstream, 1, 4)!=0x03) return false;
   // CRC
   if (crc8(bitstream, 6, 0x31, 0)!=subBitStream64(bitstream, 64-7-8, 8)) return false; 
   // CheckSum
   if (checkSum(bitstream, 7)!=subBitStream64(bitstream, 64-7, 8)) return false;
   // Data
   int id   = subBitStream64(bitstream, 5, 8);
   int bat  = subBitStream64(bitstream, 13, 1);
   int temp = subBitStream64(bitstream, 14, 11) - 400;
   int rain = (subBitStream64(bitstream, 33, 8) << 8 | subBitStream64(bitstream, 25, 8)) * 3;

   //==================================================================================
   // Output
   //==================================================================================
   display_Header();
   display_Name(PSTR(PLUGIN_DESC_101));
   display_IDn(id, 2);
   display_TEMP(temp);
   display_RAIN(rain);
   display_BAT(bat);
   display_Footer();
   //==================================================================================
   RawSignal.Repeats = true; // suppress repeats of the same RF packet
   RawSignal.Number = 0;
   return true;
}

unsigned long int subBitStream64(unsigned long long int bistream, byte debut, byte longueur) // Début = 1...
{
   bistream = bistream << (debut-1);
   bistream = bistream >> (64-longueur);
   return (unsigned long int) bistream;
}

byte checkSum(unsigned long long int bitstream, byte nBytes) // Début = 1...
{
   byte checkSum=0;
   for (byte byte=0; byte<nBytes; byte++)
      checkSum+=subBitStream64(bitstream, 1+byte*8, 8);
   return checkSum;
}

uint8_t crc8(unsigned long long int bitstream, unsigned nBytes, uint8_t polynomial, uint8_t init)
{
    uint8_t remainder = init;
    unsigned byte, bit;
    for (byte = 0; byte < nBytes; ++byte)
    {
        remainder ^= subBitStream64(bitstream, 1+byte*8, 8);
        for (bit = 0; bit < 8; ++bit)
        {
            if (remainder & 0x80) remainder = (remainder << 1) ^ polynomial;
            else remainder = (remainder << 1);
        }
    }
    return remainder;
}

void printBitStream(unsigned long long int bitstream)
{
   int nbit;
   for (int i=63; i>=0; i--)
      {
         nbit=(int) ((bitstream >> i) & 0x1);
         Serial.print(nbit);
      }
   Serial.println();
}
*/
#endif // PLUGIN_101
