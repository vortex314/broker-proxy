#include <broker_protocol.h>
#include <log.h>
#include <limero.h>
#include <ppp_frame.h>
#include <util.h>
#include <ReflectToCbor.h>
#include <ReflectFromCbor.h>
#include <CborDump.h>
#include <BrokerAbstract.h>

namespace broker
{
//-----------------------------------------------------------------------
  class Resource
  {
    int _id;
    TopicName _key;

  public:
    Resource(int i, TopicName &k) : _id(i), _key(k){};
    int id() { return _id; }
    const TopicName &key() { return _key; }
  };
//-----------------------------------------------------------------------
  template <typename T>
  class Sub : public Source< T>, public Resource
  {
    CborDeserializer _cborDeserializer;
    BrokerAbstract& _brokerAbstract;
    T _t;
  public:
    Sub(BrokerAbstract& brokerAbstract,int id, TopicName &key)
        : Source<T>(),Resource(id, key){};
    void newBytes(const Bytes& bs) {
      _cborDeserializer.fromBytes(bs) >> _t;
      if ( _cborDeserializer.success() ) emit(_t);
    }
  };
//-----------------------------------------------------------------------
  template <typename T>
  class Pub : public Sink<T>, public Resource
  {
   CborSerializer _cborSerializer(100);
  BrokerAbstract& _brokerAbstract;
  Bytes _bs;

  public:
    Pub(BrokerAbstract& brokerAbstract,int rid, TopicName &key) 
        : Sink<T>(3),Resource(rid, key){};
        void on(const T& t)  {
                                 _bs = (_cborSerializer.begin() << t).end().toBytes();
                                 _brokerAbstract.publish(id(),_bs);
                               }),
          
  };
//-----------------------------------------------------------------------

  class BrokerReactive : public Actor
  {
  public:
    BrokerAbstract& _brokerAbstract;
    ValueFlow<Bytes> incomingPublish;
    ValueFlow<Bytes> incomingConnect;
    ValueFlow<Bytes> incomingDisconnect;

    int _resourceId = 0;

    std::vector<Resource *> _publishers;
    std::vector<Resource *> _subscribers;
    TopicName brokerSrcPrefix = "src/node/";
    TopicName brokerDstPrefix = "dst/node/";

  public:
    Broker(BrokerAbstract& brokerAbstract,Thread thr) : ,_brokerAbstract(brokerAbstract),Actor(thr){};
    template <typename T>
    Sub<T> &subscriber(TopicName);
    template <typename T>
    Pub<T> &publisher(TopicName);
  };
//-----------------------------------------------------------------------

  template <typename T>
  Pub<T> &Broker::publisher(TopicName name)
  {
    TopicName topic = name;
    if (!(topic.startsWith("src/") || topic.startsWith("dst/")))
    {
      topic = brokerSrcPrefix + name;
    }
    Pub<T> *p = new Pub<T>(*this,_resourceId++, topic);
    _publishers.push_back(p);
    LOGI << " created publisher : " << name.c_str() << LEND;
    return *p;
  }
//-----------------------------------------------------------------------

  template <typename T>
  Sub<T> &Broker::subscriber(TopicName name)
  {
    TopicName topic = name;
    if (!(topic.startsWith("src/") || topic.startsWith("dst/")))
    {
      topic = brokerDstPrefix + name;
    }
    Sub<T> *s = new Sub<T>(*this,_resourceId++, topic);
    _subscribers.push_back(s);
    LOGI << " created subscriber :" << name.c_str() << LEND;
    return *s;
  }
}; // namespace broker
