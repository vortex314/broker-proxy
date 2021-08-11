#ifndef _ZENOH_SESSION_H_
#define _ZENOH_SESSION_H_
#include "limero.h"
#include <log.h>
#include "BrokerAbstract.h"
extern "C" {
#include "zenoh.h"
#include "zenoh/net.h"
}
#define US_WEST "tcp/us-west.zenoh.io:7447"
#define US_EAST "tcp/us-east.zenoh.io:7447"

using namespace std;

struct PubMsg {
  int id;
  Bytes value;
};

struct Sub {
  int id;
  string key;
  std::function<void(int,string&, const Bytes &)> callback;
  zn_subscriber_t *zn_subscriber;
};

struct Pub {
  int id;
  string key;
  zn_reskey_t zn_reskey;
  zn_publisher_t *zn_publisher;
};

class BrokerZenoh : public BrokerAbstract {
  zn_session_t *_zenoh_session;
  unordered_map<int, Sub *> _subscribers;
  unordered_map<int, Pub *> _publishers;
  static void subscribeHandler(const zn_sample_t *, const void *);
  zn_reskey_t resource(string topic);
  int scout();

 public:
  Source<bool> connected();

  BrokerZenoh(Thread &, Config &);
  int init();
  int connect(string);
  int disconnect();
  int publisher(int, string);
  int subscriber(int, string, std::function<void(int,string&, const Bytes &)>);
  int publish(int, Bytes &);
  int onSubscribe(SubscribeCallback);
  int unSubscribe(int);
  vector<PubMsg> query(string);
};

// namespace zenoh
#endif  // _ZENOH_SESSION_h_