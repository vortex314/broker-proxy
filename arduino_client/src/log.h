#ifndef C6B3C6F0_EFD2_46C1_BD00_5AA4B69BDDCC
#define C6B3C6F0_EFD2_46C1_BD00_5AA4B69BDDCC
#include <Sys.h>
#include <context.h>
#include <etl/string.h>
#include <etl/string_stream.h>
#include <stdarg.h>
using cstr = const char *const;

static constexpr cstr past_last_slash(cstr str, cstr last_slash) {
  return *str == '\0'  ? last_slash
         : *str == '/' ? past_last_slash(str + 1, str + 1)
                       : past_last_slash(str + 1, last_slash);
}

static constexpr cstr past_last_slash(cstr str) {
  return past_last_slash(str, str);
}
#ifndef __SHORT_FILE__
#define __SHORT_FILE__                              \
  ({                                                \
    constexpr cstr sf__{past_last_slash(__FILE__)}; \
    sf__;                                           \
  })
#endif
using namespace etl;

String stringFormat(const char *fmt, ...);
String hexDump(Bytes, const char *spacer = " ");
String charDump(Bytes);

#undef LEND
extern struct endl {
} LEND;

class LogS : public etl::string_stream {
  etl::string<256> myLog;

 public:
  LogS() : string_stream(myLog) {}
  void operator<<(struct endl x) { flush(); }
  template <class T>
  LogS &operator<<(const T &t) {
    (etl::string_stream &)(*this) << t;
    return *this;
  }
  template <class T>
  LogS &operator<<(String &t) {
    (etl::string_stream &)(*this) << t.c_str();
    return *this;
  }
  void flush() {
#ifdef __ARDUINO__
    Serial.println(str().c_str());
#else
    fprintf(stdout, "%s\n", str().c_str());
#endif
    myLog = "";
  }
  void log(char level, const char *file, uint32_t line, const char *function,
           const char *fmt, ...) {
    *this << Sys::millis() << " I " << file << ":" << line << fmt << LEND;
  }
};

extern LogS logger;
#define LOGI \
  logger << Sys::millis() << " I " << __SHORT_FILE__ << ":" << __LINE__ << " | "
#define LOGW \
  logger << Sys::millis() << " W " << __SHORT_FILE__ << ":" << __LINE__ << " | "
#define CHECK LOGI << " so far so good " << LEND

#define INFO(fmt, ...) LOGI << stringFormat(fmt, ##__VA_ARGS__) << LEND;
#define WARN(fmt, ...) LOGW << stringFormat(fmt, ##__VA_ARGS__) << LEND;

#endif /* C6B3C6F0_EFD2_46C1_BD00_5AA4B69BDDCC */
