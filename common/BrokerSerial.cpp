
#include "BrokerSerial.h"
const int MsgPublish::TYPE;
const int MsgPublisher::TYPE;
const int MsgSubscriber::TYPE;
const int MsgConnect::TYPE;
const int MsgDisconnect::TYPE;
//================================================================
class MsgFilter : public LambdaFlow<Bytes, Bytes>
{
  int _msgType;
  MsgBase msgBase;
  ReflectFromCbor _fromCbor;

public:
  MsgFilter(int msgType)
      : LambdaFlow<Bytes, Bytes>([this](Bytes &out, const Bytes &in)
                                 {
                                   if (msgBase.reflect(_fromCbor.fromBytes(in)).success() &&
                                       msgBase.msgType == _msgType)
                                   {
                                     out = in;
                                     return true;
                                   }
                                   return false;
                                 }),
        _fromCbor(50)
  {
    _msgType = msgType;
  };
  static MsgFilter &nw(int msgType) { return *new MsgFilter(msgType); }
};

BrokerSerial::BrokerSerial(Thread &thr, Stream &serial)
    : Broker(thr),
      _serial(serial),
      _toCbor(100), _fromCbor(100),
      keepAliveTimer(thr, 1000, true),
      connectTimer(thr, 10000, true) {}
BrokerSerial::~BrokerSerial() {}

void BrokerSerial::node(const char *n) { _node = n; };

void BrokerSerial::init()
{
  // outgoing
  connected = false;
  if (_node.length() == 0)
    _node = Sys::hostname();
  brokerSrcPrefix = "src/" + _node + "/";
  brokerDstPrefix = "dst/" + _node + "/";

  uptimePub = &publisher<uint64_t>("system/uptime");
  latencyPub = &publisher<uint64_t>("system/latency");

  uptimeSub = &subscriber<uint64_t>(brokerSrcPrefix + "system/uptime");

  fromSerialMsg >> [&](const Bytes &bs)
  { LOGI << "RXD [" << bs.size() << "] " << cborDump(bs).c_str() << LEND; };
  toSerialMsg >> [&](const Bytes &bs)
  { LOGI << "TXD [" << bs.size() << "] " << cborDump(bs).c_str() << LEND; };

  toSerialMsg >> _frameToBytes >> [&](const Bytes &bs)
  { _serial.write(bs.data(), bs.size()); };

  serialRxd >> _bytesToFrame >> fromSerialMsg;
  fromSerialMsg >> MsgFilter::nw(B_PUBLISH) >> incomingPublish;
  fromSerialMsg >> MsgFilter::nw(B_CONNECT) >> incomingConnect;
  fromSerialMsg >> MsgFilter::nw(B_DISCONNECT) >> incomingDisconnect;

  keepAliveTimer >> [&](const TimerMsg &tm)
  {
    if (connected())
    {
      uptimePub->on(Sys::millis());
    }
    else
    {
      MsgConnect msgConnect = {brokerSrcPrefix};
      toSerialMsg.on(msgConnect.reflect(_toCbor).toBytes());
    }
  };
  incomingConnect >>
      [&](const Bytes &in)
  {
    connected = true;
    LOGI << " subscribers :" << _subscribers.size()
         << "publishers :  " << _publishers.size();
    for (auto sub : _subscribers)
    {
      MsgSubscriber msgSubscriber = {sub->id(), sub->key()};
      toSerialMsg.on(msgSubscriber.reflect(_toCbor).toBytes());
    }
    for (auto pub : _publishers)
    {
      MsgPublisher msgPublisher = {pub->id(), pub->key()};
      toSerialMsg.on(msgPublisher.reflect(_toCbor).toBytes());
    }
    CHECK;
  };

  *uptimeSub >> [&](const uint64_t &t)
  {
    latencyPub->on(Sys::millis() - t);
    _loopbackReceived = Sys::millis();
  };

  connectTimer >> [&](const TimerMsg &tm)
  {
    uint64_t timeSinceLoopback = Sys::millis() - _loopbackReceived;
    if (timeSinceLoopback > 5000)
      connected = false;
  };

  _serial.setTimeout(0);
}

void BrokerSerial::onRxd(void *me)
{
  BrokerSerial *brk = (BrokerSerial *)me;
  Bytes data;
  while (brk->_serial.available())
  {
    data.push_back(brk->_serial.read());
  }
  brk->serialRxd.emit(data);
}
