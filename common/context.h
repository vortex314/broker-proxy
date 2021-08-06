#ifndef AF93E627_5E7E_4364_B117_1F70EE6F434E
#define AF93E627_5E7E_4364_B117_1F70EE6F434E

#include <vector>
typedef std::vector<uint8_t> Bytes;
typedef uint8_t byte;

#ifdef __ARDUINO__
#include <Arduino.h>

#ifdef __LM4F120_ARDUINO__ // some weird stuff with TIVA
#undef isinf
#undef isnan
bool isinf(float x);
bool isinf(double x);
bool isinf(long double x);
bool isnan(float x);
bool isnan(double x);
bool isnan(long double x);
#undef printf
int printf(const char *format, ...);
#undef min
#undef max
#endif

#ifdef __ESP32_ARDUINO__

#endif

typedef String ClientId;
typedef String TopicName;
typedef const char *Config;

#else // __ARDUINO__

#include <vector>
#include <string>
#include <ArduinoJson.h>
using namespace std;

typedef JsonObject Config;
typedef std::string String;
typedef std::string ClientId;
typedef std::string TopicName;


#endif


#endif /* AF93E627_5E7E_4364_B117_1F70EE6F434E */
