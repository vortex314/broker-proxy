
#ifndef C6B3C6F0_EFD2_46C1_BD00_5AA4B69BDDCC
#define C6B3C6F0_EFD2_46C1_BD00_5AA4B69BDDCC
#include <Sys.h>
#include <context.h>
#include <etl/string.h>
#include <etl/string_stream.h>
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
#include <sstream>
String stringFormat(const char *fmt, ...);
String hexDump(Bytes, const char *spacer = " ");
String charDump(Bytes);

extern struct endl {
} LEND;

class LogS {
  stringstream _ss;

 public:
  LogS(){};

  void operator<<(struct endl x) { flush(); }

  template <class T>
  LogS &operator<<(const T &t) {
    _ss << t;
    return *this;
  }

  template <class T>
  LogS &operator<<(String &t) {
    _ss << t.c_str();
    return *this;
  }

  void flush() {
    string s = _ss.str();
    fprintf(stdout, "%s\n", s.c_str());
    stringstream().swap(_ss);
  }

  void log(char level, const char *file, uint32_t line, const char *function,
           const char *fmt, ...) {
    *this << Sys::millis() << " I " << file << ":" << line << fmt << LEND;
  }
};

extern LogS logger;
#define LOGD \
  logger << Sys::millis() << " D " << __SHORT_FILE__ << ":" << __LINE__ << " | "
#define LOGI \
  logger << Sys::millis() << " I " << __SHORT_FILE__ << ":" << __LINE__ << " | "
#define LOGW \
  logger << Sys::millis() << " W " << __SHORT_FILE__ << ":" << __LINE__ << " | "
#define LOGE \
  logger << Sys::millis() << " E " << __SHORT_FILE__ << ":" << __LINE__ << " | "
#define CHECK LOGI << " so far so good " << LEND

#ifdef INFO
#undef INFO
#undef WARN
#endif
#define DEBUG(fmt, ...) LOGD << stringFormat(fmt, ##__VA_ARGS__) << LEND;
#define INFO(fmt, ...) LOGI << stringFormat(fmt, ##__VA_ARGS__) << LEND;
#define WARN(fmt, ...) LOGW << stringFormat(fmt, ##__VA_ARGS__) << LEND;
#define ERROR(fmt, ...) LOGE << stringFormat(fmt, ##__VA_ARGS__) << LEND;

#endif /* C6B3C6F0_EFD2_46C1_BD00_5AA4B69BDDCC */
