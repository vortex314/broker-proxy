#include <BrokerAbstract.h>
#include <CborDump.h>
#include <ReflectFromCbor.h>
#include <ReflectToCbor.h>
#include <broker_protocol.h>
#include <limero.h>
#include <log.h>
#include <ppp_frame.h>

class BrokerReactive;
//------------------------------------------------------------------- Resource
class Resource {
  int _id;
  TopicName _key;

 public:
  Resource(int i, TopicName &k) : _id(i), _key(k){};
  int id() { return _id; }
  const TopicName &key() { return _key; }
};
//----------------------------------------------------------------------- Sub
template <typename T>
class Sub : public Source<T>, public Resource {
  CborDeserializer _cborDeserializer;
  BrokerReactive &_brokerReactive;
  T _t;

 public:
  Sub(BrokerReactive &brokerReactive, int id, TopicName &key)
      : Source<T>(),
        Resource(id, key),
        _cborDeserializer(100),
        _brokerReactive(brokerReactive) {
  };
  void newBytes(const Bytes &bs) {
    _cborDeserializer.fromBytes(bs).begin().get(_t);
    if (_cborDeserializer.success()) emit(_t);
  }
};
//----------------------------------------------------------------------- Pub
template <typename T>
class Pub : public Sink<T>, public Resource {
  CborSerializer _cborSerializer;
  BrokerReactive &_brokerReactive;
  Bytes _bs;

 public:
  Pub(BrokerReactive &brokerReactive, int rid, TopicName &key)
      : Sink<T>(3), Resource(rid, key),_cborSerializer(100),_brokerReactive(brokerReactive){};
  void on(const T &t) {
    _bs = (_cborSerializer.begin() << t).end().toBytes();
    _brokerReactive.publish(id(), _bs);
  }
};
//-----------------------------------------------------------------------

class BrokerReactive : public Actor {
 public:
  BrokerAbstract &_brokerAbstract;
  ValueFlow<Bytes> incomingPublish;
  ValueFlow<Bytes> incomingConnect;
  ValueFlow<Bytes> incomingDisconnect;

  int _resourceId = 0;

  std::vector<Resource *> _publishers;
  std::vector<Resource *> _subscribers;
  TopicName brokerSrcPrefix = "src/node/";
  TopicName brokerDstPrefix = "dst/node/";

 public:
  BrokerReactive(BrokerAbstract &brokerAbstract, Thread thr)
      : _brokerAbstract(brokerAbstract), Actor(thr){};
  template <typename T>
  Sub<T> &subscriber(TopicName name) {
    TopicName topic = name;
    if (!(topic.rfind("src/", 0) == 0 || topic.rfind("dst/", 0) == 0)) {
      topic = brokerDstPrefix + name;
    }
    Sub<T> *s = new Sub<T>(*this, _resourceId++, topic);
    _subscribers.push_back(s);
    LOGI << " created subscriber :" << name.c_str() << LEND;
    return *s;
  }
  template <typename T>
  Pub<T> &publisher(TopicName name) {
    TopicName topic = name;
    if (!(topic.rfind("src/", 0) == 0 || topic.rfind("dst/", 0) == 0)) {
      topic = brokerSrcPrefix + name;
    }
    Pub<T> *p = new Pub<T>(*this, _resourceId++, topic);
    _publishers.push_back(p);
    LOGI << " created publisher : " << name.c_str() << LEND;
    return *p;
  }
};
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
