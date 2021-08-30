#ifndef BEA3A05D_E4F4_41BB_A864_310EE1D37C62
#define BEA3A05D_E4F4_41BB_A864_310EE1D37C62

#include <CborDeserializer.h>
#include <CborSerializer.h>
#include <limero.h>
typedef int (*SubscribeCallback)(int, Bytes);

struct PubMsg
{
  String topic;
  Bytes payload;
};

class BrokerBase : public Actor
{
  QueueFlow<PubMsg> _incoming;
  QueueFlow<PubMsg> _outgoing;
  CborSerializer _toCbor;
  CborDeserializer _fromCbor;
  ValueFlow<bool> _connected;
  friend class BrokerZenoh;
  friend class BrokerRedis;

public:
  BrokerBase(Thread &thr, Config &)
      : Actor(thr), _incoming(10, "incoming"), _outgoing(10, "outgoing"),
        _toCbor(1024), _fromCbor(1024){};
  virtual int init() = 0;
  virtual int connect(string) = 0;
  virtual int disconnect() = 0;
  virtual int publish(string , Bytes &) = 0;
  virtual int subscribe(string ) = 0;
  virtual int unSubscribe(string ) = 0;
  virtual bool match(string pattern, string source) = 0;

  Source<bool> &connected() { return _connected; };
  Source<PubMsg> &incoming() { return _incoming; };

  template <typename T>
  Sink<T> &publisher(TopicName topic)
  {
    SinkFunction<T> *sf = new SinkFunction<T>([&](const T &t)
                                              {
                                                Bytes bs = _toCbor.begin().add(t).end().toBytes();
                                                _outgoing.on({topic, bs});
                                              });
    return *sf;
  }
  template <typename T>
  Source<T> &subscriber(string pattern)
  {
    auto lf = new LambdaFlow<PubMsg, T>([&](T &t, const PubMsg &msg)
                                        {
                                          if (match(pattern, msg.topic))
                                            return _fromCbor.fromBytes(msg.payload).begin().get(t).success();
                                          else
                                            return false;
                                        });
    _incoming >> lf;
    return *lf;
  }
};

#endif /* BEA3A05D_E4F4_41BB_A864_310EE1D37C62 */
