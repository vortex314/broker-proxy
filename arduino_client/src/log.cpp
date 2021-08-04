
#include "log.h"
#include <string.h>
#include <assert.h>
String hexDump(Bytes bs, const char* spacer) {
  static char HEX_DIGITS[] = "0123456789ABCDEF";
  String out;
  for (uint8_t b : bs) {
    out += HEX_DIGITS[b >> 4];
    out += HEX_DIGITS[b & 0xF];
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

#include <string> 
String stringFormat(const char* fmt, ...) {
  static std::string str;
  str.clear();
  int size = strlen(fmt) * 2 + 50;  // Use a rubric appropriate for your code
  va_list ap;
  while (1) {  // Maximum two passes on a POSIX system...
    assert(size < 10240);
    str.resize(size);
    va_start(ap, fmt);
    int n = vsprintf((char*)str.data(), fmt, ap);
    va_end(ap);
    if (n > -1 && n < size) {  // Everything worked
      str.resize(n);
      return str.c_str();
    }
    if (n > -1)      // Needed size returned
      size = n + 1;  // For null char
    else
      size *= 2;  // Guess at a larger size (OS specific)
  }
  return str.c_str();
}