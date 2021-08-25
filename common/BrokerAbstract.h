#ifndef BEA3A05D_E4F4_41BB_A864_310EE1D37C62
#define BEA3A05D_E4F4_41BB_A864_310EE1D37C62

#include <limero.h>
#include <CborDeserializer.h>
#include <CborSerializer.h>
typedef int (*SubscribeCallback)(int, Bytes);

class BrokerAbstract {
  uint32_t _newId=1000;
 public:
  Source<bool> &connected();
  virtual int init() = 0;
  virtual int connect(string) = 0;
  virtual int disconnect() = 0;
  virtual int publisher(int, string) = 0;
  virtual int subscriber(int, string,
                         std::function<void(int, string &, const Bytes &)>) = 0;
  virtual int publish(int, Bytes &) = 0;
  virtual int unSubscribe(int) = 0;
  template <typename T>
  Sink<T> &publisher(TopicName topic) {
    SinkFunction<T> *sf = new SinkFunction<T>([&](const T &t) {
      static CborSerializer toCbor(100);
      Bytes bs = toCbor.begin().add(t).end().toBytes();
      publish(_newId++, bs);
    });
    return *sf;
  }
  template <typename T>
  Source<T> subscriber(string);
};

#endif /* BEA3A05D_E4F4_41BB_A864_310EE1D37C62 */
