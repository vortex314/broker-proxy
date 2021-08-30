#ifndef _ZENOH_SESSION_H_
#define _ZENOH_SESSION_H_
#include "BrokerBase.h"
#include "limero.h"
#include <async.h>
#include <hiredis.h>
#include <log.h>

using namespace std;



struct SubscriberStruct {
  int id;
  string pattern;
  std::function<void(int, string &, const Bytes &)> callback;
  static void onMessage(redisContext *c, void *reply, void *me);
};

struct PublisherStruct {
  int id;
  string key;
  static void onReply(redisAsyncContext *c, void *reply, void *me);
};

class BrokerRedis : public BrokerBase {
  Thread &_thread;
  unordered_map<int, SubscriberStruct *> _subscribers;
  unordered_map<int, PublisherStruct *> _publishers;
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

public:
  Source<bool> &connected();

  BrokerRedis(Thread &, Config &);
  int init();
  int connect(string);
  int disconnect();
  int publish(string&, Bytes &);
  int onSubscribe(SubscribeCallback);
  int unSubscribe(string&);
  int subscribe(string&);
  int command(const char *format, ...);
  int getId(string);
  vector<PubMsg> query(string);
  
};

// namespace zenoh
#endif // _ZENOH_SESSION_h_