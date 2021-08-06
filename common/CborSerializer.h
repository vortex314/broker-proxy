#ifndef __CBOR_SERIALIZER_H__
#define __CBOR_SERIALIZER_H__
#include <context.h>
#include <cbor.h>
#include <assert.h>

using namespace std;

class CborSerializer
{
  uint32_t _capacity;
  size_t _size;
  CborError _err;
  CborEncoder _encoderRoot;
  CborEncoder _encoder;
  uint8_t *_buffer;

public:
  CborSerializer(int size);
  ~CborSerializer();
  CborSerializer &operator<<(const int &t);
  CborSerializer &operator<<(const String &t);
  CborSerializer &operator<<(const Bytes &t);
  CborSerializer &operator<<(int &t);
  //  CborSerializer &operator<<(int32_t &t);
  CborSerializer &operator<<(uint32_t &t);
  CborSerializer &operator<<(int64_t &t);
  CborSerializer &operator<<(uint64_t &t);
  CborSerializer &operator<<(String &t);
  CborSerializer &operator<<(float &t);
  CborSerializer &operator<<(double &t);
  CborSerializer &operator<<(Bytes &t);
  CborSerializer &begin();
  CborSerializer &end();
  Bytes toBytes();
};
#endif /* E04F2BFB_223A_4990_A609_B7AA6A5E6BE8 */
