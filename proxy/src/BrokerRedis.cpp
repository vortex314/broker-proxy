
#include <BrokerRedis.h>

#include <CborDump.h>

void SubscriberStruct::onMessage(redisContext *c, void *reply, void *me) {
  SubscriberStruct *sub = (SubscriberStruct *)me;
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

void PublisherStruct::onReply(redisAsyncContext *c, void *reply, void *me) {
  PublisherStruct *sub = (PublisherStruct *)me;
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

BrokerRedis::BrokerRedis(Thread &thread, Config &cfg)
    : _thread(thread), _reconnectTimer(thread, 3000, true, "reconnectTimer") {
  _hostname = cfg["broker"]["host"] | "localhost";
  _port = cfg["broker"]["port"] | 6379;

  _connected >> [](const bool &connected) {
    LOGI << "Connection state : " << (connected ? "connected" : "disconnected")
         << LEND;
  };
  _reconnectTimer >> [&](const TimerMsg &) {
    if (!_connected())
      connect("");
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
  options.connect_timeout = new timeval{3, 0}; // 3 sec
  options.command_timeout = new timeval{3, 0}; // 3 sec
  REDIS_OPTIONS_SET_PRIVDATA(&options, this, free_privdata);

  LOGI << "Connecting to Redis " << _hostname << ":" << _port << LEND;
  _subscribeContext = redisConnectWithOptions(&options);
  if (_subscribeContext == NULL || _subscribeContext->err ) {
    LOGW << " Connection " << _hostname << ":" << _port << "  failed." << _subscribeContext->errstr << LEND;
    return ENOTCONN;
  } 
  _thread.addReadInvoker(_subscribeContext->fd, [&](int) {
    redisReply *reply;
    int rc = redisGetReply(_subscribeContext, (void **)&reply);
    if (rc == 0) {
      if (reply->type == REDIS_REPLY_ARRAY &&
          strcmp(reply->element[0]->str, "pmessage") == 0) {
        SubscriberStruct *sub = findSub(reply->element[1]->str);
        if (sub)
          SubscriberStruct::onMessage(_subscribeContext, reply, sub);
      } else {
        INFO(" reply not handled ");
      }

      freeReplyObject(reply);
    } else {
      INFO(" reply not found ");
      disconnect();
    }
  });
  _publishContext = redisConnectWithOptions(&options);
  if (_publishContext == NULL)
    return ENOTCONN;
  _connected = true;
  _publishContext->privdata = this;
  _subscribeContext->privdata = this;
  return 0;
}

int BrokerRedis::disconnect() {
  LOGI << (" disconnecting.") << LEND;
  if (!_connected())
    return 0;
  _thread.deleteInvoker(_subscribeContext->fd);
  redisFree(_publishContext);
  redisFree(_subscribeContext);

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
    SubscriberStruct *sub = new SubscriberStruct({id, pattern, callback});
    redisReply *r = (redisReply *)redisCommand(
        _subscribeContext, "PSUBSCRIBE %s", pattern.c_str());
    if (r) {
      LOGI << " PSUBSCRIBE : " << pattern << " created." << LEND;
      _subscribers.emplace(id, sub);
      freeReplyObject(r);
    }
  }
  return 0;
}

int BrokerRedis::unSubscribe(int id) {
  auto it = _subscribers.find(id);
  if (it == _subscribers.end()) {
  } else {
    redisReply *r = (redisReply *)redisCommand(
        _subscribeContext, "PUNSUBSCRIBE %s", it->second->pattern);
    if (r) {
      LOGI << " PUNSUBSCRIBE : " << it->second->pattern << " created." << LEND;
      _subscribers.erase(id);
      freeReplyObject(r);
    }
  }
  return 0;
}

int BrokerRedis::publisher(int id, string key) {
  if (_publishers.find(id) == _publishers.end()) {
    PublisherStruct *pPub = new PublisherStruct{id, key};
    _publishers.emplace(id, pPub);
    LOGI << " Publisher : " << key << " created." << LEND;
  }
  return 0;
}

int BrokerRedis::publish(int id, Bytes &bs) {
  if (!_connected())
    return ENOTCONN;
  auto it = _publishers.find(id);
  if (it != _publishers.end()) {
    redisReply *r = (redisReply *)redisCommand(_publishContext, "PUBLISH %s %b",
                                               it->second->key.c_str(),
                                               bs.data(), bs.size());
    if (r == 0) {
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

SubscriberStruct *BrokerRedis::findSub(string pattern) {
  for (auto it : _subscribers) {
    if (it.second->pattern == pattern)
      return it.second;
  }
  return 0;
}

int BrokerRedis::command(const char *format, ...) {
  if (!_connected())
    return ENOTCONN;
  va_list ap;
  va_start(ap, format);
  void *reply = redisvCommand(_publishContext, format, ap);
  va_end(ap);
  if (reply) {
    LOGI << " command : " << format << LEND;
    freeReplyObject(reply);
    return 0;
  }
  LOGW << "command : " << format << " failed " << LEND;
  return EINVAL;
}