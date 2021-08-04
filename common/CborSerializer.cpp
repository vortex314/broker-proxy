#include <CborSerializer.h>
#include <cbor.h>
#include <util.h>
#include <log.h>

#undef assert
#define assert(xxx) if (!(xxx)) LOGW << " assert failed " << #xxx << LEND;

CborSerializer::CborSerializer(int size) {
  _buffer = new uint8_t[size];
  _capacity = size;
  //    cbor_encoder_init(&_encoderRoot, _buffer, _capacity, 0);
  //   cbor_encoder_create_array(&_encoderRoot, &_encoder,
  //   CborIndefiniteLength);
}
CborSerializer::~CborSerializer(){
  delete[] _buffer;
}
CborSerializer &CborSerializer::operator<<( const int &t) {
  _err = cbor_encode_int(&_encoder, t);
  assert(_err == 0);
  return *this;
}
CborSerializer &CborSerializer::operator<<( int &t) {
  _err = cbor_encode_int(&_encoder, t);
  assert(_err == 0);
  return *this;
}
CborSerializer &CborSerializer::operator<<( uint64_t &t) {
  _err = cbor_encode_uint(&_encoder, t);
  assert(_err == 0);
  return *this;
}
CborSerializer &CborSerializer::operator<<( int64_t &t) {
  _err = cbor_encode_int(&_encoder, t);
  assert(_err == 0);
  return *this;
}
CborSerializer &CborSerializer::operator<<( String &t) {
  _err = cbor_encode_text_string(&_encoder, t.c_str(), t.length());
  return *this;
}

CborSerializer &CborSerializer::operator<<( const String &t) {
  _err = cbor_encode_text_string(&_encoder, t.c_str(), t.length());
  return *this;
}

CborSerializer &CborSerializer::operator<<(double &t) {
  _err = cbor_encode_double(&_encoder, t);
  assert(_err == 0);
  return *this;
}

CborSerializer &CborSerializer::operator<<( Bytes &t) {
  _err = cbor_encode_byte_string(&_encoder, t.data(), t.size());
  assert(_err == 0);
  return *this;
}

CborSerializer &CborSerializer::operator<<( const Bytes &t) {
  return *this << (Bytes&) t;
}

CborSerializer &CborSerializer::begin() {
  cbor_encoder_init(&_encoderRoot, _buffer, _capacity, 0);
  _err =
      cbor_encoder_create_array(&_encoderRoot, &_encoder, CborIndefiniteLength);
  assert(_err == 0);
  return *this;
}

CborSerializer &CborSerializer::end() {
  _err = cbor_encoder_close_container(&_encoderRoot, &_encoder);
  assert(_err == 0);
  _size = cbor_encoder_get_buffer_size(&_encoderRoot, _buffer);
  return *this;
}

Bytes CborSerializer::toBytes() { return Bytes(_buffer, _buffer + _size); }
