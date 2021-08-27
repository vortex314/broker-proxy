#ifndef _ZENOH_SESSION_H_
#define _ZENOH_SESSION_H_
#include <log.h>

#include "BrokerAbstract.h"
#include "limero.h"
extern "C" {
#include "zenoh.h"
#include "zenoh/net.h"
}
#define US_WEST "tcp/us-west.zenoh.io:7447"
#define US_EAST "tcp/us-east.zenoh.io:7447"

using namespace std;

struct SubscriberStruct {
  string pattern;
  std::function<void(string &, const Bytes &)> callback;
  zn_subscriber_t *zn_subscriber;
};

struct PublisherStruct {
  string key;
  zn_reskey_t zn_reskey;
  zn_publisher_t *zn_publisher;
};

class BrokerZenoh : public BrokerAbstract {
  zn_session_t *_zenoh_session;
  unordered_map<string, SubscriberStruct *> _subscribers;
  unordered_map<string, PublisherStruct *> _publishers;
  zn_reskey_t resource(string topic);
  int scout();
  ValueFlow<bool> _connected;
  QueueFlow<PubMsg> _incoming;
  
  PublisherStruct* publisher(string pattern) ;
  static void subscribeHandler(const zn_sample_t *, const void *);


 public:
  BrokerZenoh(Thread &, Config &);
  int init();
  int connect(string);
  int disconnect();
  int publish(string &, Bytes &);
  int subscribe(string &);
  int unSubscribe(int);
  vector<PubMsg> query(string);
  Source<PubMsg> &incoming() { return _incoming; };
  Source<bool> &connected() { return _connected; };
};

// namespace zenoh
#endif  // _ZENOH_SESSION_h_