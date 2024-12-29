/////////////////////////////////////////////
//!! Credentials Personnels - gitignored !!//
/////////////////////////////////////////////

#include <Arduino.h>

String FiltreMac(String Mac)
{
  String NoSerie;
  NoSerie.reserve(Mac.length()); // optional, avoids buffer reallocations in the loop
  for(size_t i = 0; i < Mac.length(); ++i) if(Mac[i] != ':') NoSerie += Mac[i];
  return NoSerie;
}