#ifndef __CBOR_SERIALIZER_H__
#define __CBOR_SERIALIZER_H__
#include <assert.h>
#include <cbor.h>
#include <context.h>
#include <logger.h>

using namespace std;

class CborSerializer {
  uint32_t _capacity;
  size_t _size;
  CborError _err;
  CborEncoder _encoderRoot;
  CborEncoder _encoder;
  uint8_t *_buffer;

public:
  CborSerializer(int size);
  ~CborSerializer();
  template <typename T> CborSerializer &add(T t) {
    *this << t;
    return *this;
  }

  CborSerializer &operator<<(int t);
  //  CborSerializer &operator<<(int32_t &t);
  CborSerializer &operator<<(uint32_t t);
  CborSerializer &operator<<(int64_t t);
  CborSerializer &operator<<(uint64_t t);
  CborSerializer &operator<<(String t);
  CborSerializer &operator<<(float t);
  CborSerializer &operator<<(double t);
  CborSerializer &operator<<(Bytes t);
  CborSerializer &begin();
  CborSerializer &end();
  bool success();
  Bytes toBytes();
};
#endif /* E04F2BFB_223A_4990_A609_B7AA6A5E6BE8 */
