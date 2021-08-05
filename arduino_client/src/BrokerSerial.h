#ifndef BrokerSerial_H
#define BrokerSerial_H
#include <ReflectFromCbor.h>
#include <ReflectToCbor.h>
#include "broker.h"
#include <broker_protocol.h>
#include <limero.h>
#include <Frame.h>
#include <CborDump.h>
#include <util.h>

class BrokerSerial : public broker::Broker
{
  Stream &_serial;
  ValueSource<Bytes> serialRxd;
  broker::Publisher<uint64_t> *uptimePub;
  broker::Publisher<uint64_t> *latencyPub;
  broker::Subscriber<uint64_t> *uptimeSub;

  BytesToFrame _bytesToFrame;
  FrameToBytes _frameToBytes;
  ReflectToCbor _toCbor;
  ReflectFromCbor _fromCbor;

  TopicName _loopbackTopic = "dst/esp32/system/loopback";
  TopicName _dstPrefix = "dst/esp32/";
  uint64_t _loopbackReceived;
  String _node;

public:
  ValueSource<bool> connected;
  static void onRxd(void *);
  TimerSource keepAliveTimer;
  TimerSource connectTimer;
  BrokerSerial(Thread &thr, Stream &serial);
  ~BrokerSerial();
  void init();
  void node(const char*);
};

#endif // BrokerSerial_H
