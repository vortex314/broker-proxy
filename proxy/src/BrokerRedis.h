#ifndef _ZENOH_SESSION_H_
#define _ZENOH_SESSION_H_
#include "BrokerAbstract.h"
#include "limero.h"
#include <adapters/libevent.h>
#include <async.h>
#include <hiredis.h>
#include <log.h>

using namespace std;

struct PubMsg {
  int id;
  Bytes value;
};

struct Sub {
  int id;
  string key;
  std::function<void(int, string &, const Bytes &)> callback;
  static void onMessage(redisAsyncContext *c, void *reply, void *me);
};

struct Pub {
  int id;
  string key;
  static void onReply(redisAsyncContext *c, void *reply, void *me);
};

class BrokerRedis : public BrokerAbstract {
  unordered_map<int, Sub *> _subscribers;
  unordered_map<int, Pub *> _publishers;
  int scout();
  string _hostname;
  uint16_t _port;
  redisAsyncContext *_subscribeContext;
  redisAsyncContext *_publishContext;
  ValueFlow<bool> _connected;
  Thread *_publishEventThread;
  Thread *_subscribeEventThread;
  struct event_base *_publishEventBase;
  struct event_base *_subscribeEventBase;

public:
  Source<bool> &connected();

  BrokerRedis(Thread &, Config &);
  int init();
  int connect(string);
  int disconnect();
  int publisher(int, string);
  int subscriber(int, string,
                 std::function<void(int, string &, const Bytes &)>);
  int publish(int, Bytes &);
  int onSubscribe(SubscribeCallback);
  int unSubscribe(int);
  vector<PubMsg> query(string);
};

// namespace zenoh
#endif // _ZENOH_SESSION_h_