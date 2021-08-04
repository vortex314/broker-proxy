#include "util.h"

String hexDump(Bytes bs,const char* spacer) {
  static char HEX_VALUES[] = "0123456789ABCDEF";
  String out;
  for (uint8_t b : bs) {
    out += HEX_VALUES[b >> 4];
    out += HEX_VALUES[b & 0xF];
    out += spacer;
  }
  return out;
}

String charDump(Bytes bs) {
  String out;
  for (uint8_t b : bs) {
    if (isprint(b))
      out += (char)b;
    else
      out += '.';
  }
  return out;
}