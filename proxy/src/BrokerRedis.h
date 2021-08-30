#ifndef _ZENOH_SESSION_H_
#define _ZENOH_SESSION_H_
#include <async.h>
#include <hiredis.h>
#include <log.h>

#include "BrokerBase.h"
#include "limero.h"

using namespace std;

struct SubscriberStruct {
  string pattern;
};

class BrokerRedis : public BrokerBase {
  Thread &_thread;
  unordered_map<string, SubscriberStruct *> _subscribers;
  int scout();
  string _hostname;
  uint16_t _port;
  redisContext *_subscribeContext;
  redisContext *_publishContext;
  ValueFlow<bool> _connected;
  Thread *_publishEventThread;
  Thread *_subscribeEventThread;
  struct event_base *_publishEventBase;
  struct event_base *_subscribeEventBase;
  SubscriberStruct *findSub(string pattern);
  TimerSource _reconnectTimer;
  static void onMessage(redisContext *c, void *reply, void *me);

 public:
  Source<bool> &connected();

  BrokerRedis(Thread &, Config &);
  int init();
  int connect(string);
  int disconnect();
  int publish(string , Bytes &);
  int onSubscribe(SubscribeCallback);
  int unSubscribe(string );
  int subscribe(string );
  bool match(string pattern, string source);

  int command(const char *format, ...);
  int getId(string);
  int newRedisPublisher(string topic);
  vector<PubMsg> query(string);
};

// namespace zenoh
#endif  // _ZENOH_SESSION_h_