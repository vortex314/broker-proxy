#ifndef AA72A63D_3140_4FF2_BCAF_6F744DBBD62B
#define AA72A63D_3140_4FF2_BCAF_6F744DBBD62B
#include <context.h>
#include <logger.h>
#include <cbor.h>
#include <assert.h>

class CborDeserializer
{
  CborParser _decoder;
  CborValue _rootIt, _it;
  CborError _err;
  uint8_t *_buffer;
  size_t _size;
  size_t _capacity;

public:
  CborDeserializer(size_t size);
  ~CborDeserializer();
  CborDeserializer &begin();
  CborDeserializer &end();
  CborDeserializer &operator>>(Bytes &t);
  CborDeserializer &operator>>(String &t);
  CborDeserializer &operator>>(int &t);
  //  CborDeserializer &operator>>(int32_t &t, const char *n="",const char *d="");
  CborDeserializer &operator>>(uint32_t &t);
  CborDeserializer &operator>>(int64_t &t);
  CborDeserializer &operator>>(uint64_t &t);
  CborDeserializer &operator>>(float &t);
  CborDeserializer &operator>>(double &t);
  CborDeserializer &operator>>(const int &t);
  CborDeserializer &fromBytes(const Bytes &bs);
  bool success();
};
#endif /* AA72A63D_3140_4FF2_BCAF_6F744DBBD62B */
