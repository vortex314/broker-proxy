
#include <BrokerRedis.h>

#include <CborDump.h>
/*
1) "pmessage"
2) "src/*"
3) "src/esp32/aa"
4) "123"

*/
void Sub::onMessage(redisAsyncContext *c, void *reply, void *me) {
  Sub *sub = (Sub *)me;
  if (reply == NULL)
    return;
  redisReply *r = (redisReply *)reply;
  if (r->type == REDIS_REPLY_ARRAY) {
    /*    stringstream ss;
        for (int i = 0; i < r->elements; i++) {
          ss << r->element[i]->str << ",";
        }
        LOGI << ss.str() << LEND;*/
    if (strcmp(r->element[0]->str, "pmessage") == 0) {
      string topic = r->element[2]->str;
      sub->callback(
          sub->id, topic,
          Bytes(r->element[3]->str, r->element[3]->str + r->element[3]->len));
    } else {
      LOGW << "unexpected array " << r->element[0]->str << LEND;
    }
  } else {
    LOGW << " unexpected reply " << LEND;
  }
}

void Pub::onReply(redisAsyncContext *c, void *reply, void *me) {
  Pub *sub = (Pub *)me;
  redisReply *r = (redisReply *)reply;
  if (r->type == REDIS_REPLY_ARRAY) {
    for (int j = 0; j < r->elements; j++) {
      LOGI << j << ":" << r->element[j]->str << LEND;
    }
  } else if (r->type == REDIS_REPLY_INTEGER) {
    LOGD << " integer received " << r->integer << LEND;
  } else {
    LOGW << " unexpected reply elements:" << r->elements << " type:" << r->type
         << LEND;
  }
}

class EventInvoker : public Invoker {
  struct event_base *_base;

public:
  EventInvoker(struct event_base *base) : _base(base){};
  void invoke() {
    while (true)
      event_base_dispatch(_base);
  }
};
#include <event2/thread.h>
BrokerRedis::BrokerRedis(Thread &thr, Config &cfg) {
  _hostname = cfg["broker"]["host"] | "localhost";
  _port = cfg["broker"]["port"] | 6379;
  evthread_use_pthreads();

  _publishEventBase = event_base_new();
  _publishEventThread = new Thread("event Publish Thread ");
  _publishEventThread->enqueue(new EventInvoker(_publishEventBase));
  _publishEventThread->start();

  _subscribeEventBase = event_base_new();
  _subscribeEventThread = new Thread("event Subscribe Thread ");
  _subscribeEventThread->enqueue(new EventInvoker(_subscribeEventBase));
  _subscribeEventThread->start();

  _connected >> [](const bool &connected) {
    LOGI << "Connection state : " << (connected ? "connected" : "disconnected")
         << LEND;
  };
}

void free_privdata(void *pvdata) {}

int BrokerRedis::init() { return 0; }

int BrokerRedis::connect(string clientId) {
  if (_connected()) {
    _connected = true;
    return 0;
  }
  redisOptions options = {0};
  REDIS_OPTIONS_SET_TCP(&options, _hostname.c_str(), _port);
  REDIS_OPTIONS_SET_PRIVDATA(&options, this, free_privdata);

  LOGI << "Connecting to Redis " << _hostname << ":" << _port << LEND;
  _subscribeContext = redisAsyncConnectWithOptions(&options);
  if (_subscribeContext == NULL)
    return ENOTCONN;
  _publishContext = redisAsyncConnectWithOptions(&options);
  if (_publishContext == NULL)
    return ENOTCONN;
  _publishContext->data = this;
  _subscribeContext->data = this;
  redisLibeventAttach(_subscribeContext, _subscribeEventBase);
  redisLibeventAttach(_publishContext, _publishEventBase);

  redisAsyncSetConnectCallback(_publishContext,
                               [](const redisAsyncContext *ac, int status) {
                                 BrokerRedis *me = (BrokerRedis *)ac->data;
                               });
  redisAsyncSetConnectCallback(
      _subscribeContext, [](const redisAsyncContext *ac, int status) {
        BrokerRedis *me = (BrokerRedis *)ac->data;
        me->_connected = status == REDIS_OK ? true : false;
      });
  redisAsyncSetDisconnectCallback(_publishContext,
                                  [](const redisAsyncContext *ac, int status) {
                                    BrokerRedis *me = (BrokerRedis *)ac->data;
                                    me->disconnect();
                                  });

  return 0;
}

int BrokerRedis::disconnect() {
  LOGI << (" disconnecting.") << LEND;
  if (!_connected())
    return 0;
  redisAsyncDisconnect(_publishContext);
  redisAsyncDisconnect(_subscribeContext);

  for (auto tuple : _subscribers) {
  }
  _subscribers.clear();
  for (auto tuple : _publishers) {
  }
  _publishers.clear();
  _connected = false;
  return 0;
}

int BrokerRedis::subscriber(
    int id, string pattern,
    std::function<void(int, string &, const Bytes &)> callback) {
  if (_subscribers.find(id) == _subscribers.end()) {
    Sub *sub = new Sub({id, pattern, callback});
    int rc = redisAsyncCommand(_subscribeContext, Sub::onMessage, sub,
                               "PSUBSCRIBE %s", pattern.c_str());
    LOGI << " PSUBSCRIBE : " << pattern << " created." << LEND;
    _subscribers.emplace(id, sub);
  }
  return 0;
}

int BrokerRedis::unSubscribe(int id) {
  auto it = _subscribers.find(id);
  if (it == _subscribers.end()) {
  } else {
    int rc = redisAsyncCommand(_subscribeContext, Sub::onMessage, it->second,
                               "PUNSUBSCRIBE %s", it->second->key.c_str());
    LOGI << " PUNSUBSCRIBE : " << it->second->key << " created." << LEND;
    _subscribers.erase(id);
  }
  return 0;
}

int BrokerRedis::publisher(int id, string key) {
  if (_publishers.find(id) == _publishers.end()) {
    Pub *pPub = new Pub{id, key};
    _publishers.emplace(id, pPub);
    LOGI << " Publisher : " << key << " created." << LEND;
  }
  return 0;
}

int BrokerRedis::publish(int id, Bytes &bs) {
  auto it = _publishers.find(id);
  if (it != _publishers.end()) {
    int rc = redisAsyncCommand(_publishContext, Pub::onReply, it->second,
                               "PUBLISH %s %b", it->second->key.c_str(),
                               bs.data(), bs.size());
    if (rc) {
      LOGW << "PUBLISH failed " << it->second->key.c_str() << LEND;
    } else {
      LOGD << " PUBLISH : " << it->second->key << ":" << cborDump(bs) << LEND;
    }
    return 0;
  } else {
    INFO(" publish id %d unknown in %d publishers. ", id, _publishers.size());
    return ENOENT;
  }
}

Source<bool> &BrokerRedis::connected() { return _connected; }