
#include <BrokerRedis.h>
#include <CborDump.h>

void BrokerRedis::onMessage(redisContext *c, void *reply, void *me) {
  BrokerRedis *pBroker = (BrokerRedis *)me;
  if (reply == NULL) return;
  redisReply *r = (redisReply *)reply;
  if (r->type == REDIS_REPLY_ARRAY) {
    if (strcmp(r->element[0]->str, "pmessage") == 0) {
      string topic = r->element[2]->str;
      pBroker->_incoming.on(
          {topic,
           Bytes(r->element[3]->str, r->element[3]->str + r->element[3]->len)});
    } else {
      LOGW << "unexpected array " << r->element[0]->str << LEND;
    }
  } else {
    LOGW << " unexpected reply " << LEND;
  }
}
/*
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
}*/

BrokerRedis::BrokerRedis(Thread &thread, Config &cfg)
    : BrokerBase(thread, cfg),
      _thread(thread),
      _reconnectTimer(thread, 3000, true, "reconnectTimer") {
  _hostname = cfg["broker"]["host"] | "localhost";
  _port = cfg["broker"]["port"] | 6379;

  connected >> [](const bool &connected) {
    LOGI << "Connection state : " << (connected ? "connected" : "disconnected")
         << LEND;
  };
  connected = false;
  _reconnectTimer >> [&](const TimerMsg &) {
    if (!connected()) connect("");
  };
}

void free_privdata(void *pvdata) {}

int BrokerRedis::init() { return 0; }

int BrokerRedis::connect(string clientId) {
  if (connected()) {
    LOGI << " Connecting but already connected." << LEND;
    connected = true;
    return 0;
  }
  redisOptions options = {0};
  REDIS_OPTIONS_SET_TCP(&options, _hostname.c_str(), _port);
  options.connect_timeout = new timeval{3, 0};  // 3 sec
  options.command_timeout = new timeval{3, 0};  // 3 sec
  REDIS_OPTIONS_SET_PRIVDATA(&options, this, free_privdata);

  LOGI << "Connecting to Redis " << _hostname << ":" << _port << LEND;
  _subscribeContext = redisConnectWithOptions(&options);
  if (_subscribeContext == NULL || _subscribeContext->err) {
    LOGW << " Connection " << _hostname << ":" << _port << "  failed."
         << _subscribeContext->errstr << LEND;
    return ENOTCONN;
  }
  _thread.addReadInvoker(_subscribeContext->fd, [&](int) {
    redisReply *reply;
    int rc = redisGetReply(_subscribeContext, (void **)&reply);
    if (rc == 0) {
      if (reply->type == REDIS_REPLY_ARRAY &&
          strcmp(reply->element[0]->str, "pmessage") == 0) {
        onMessage(_subscribeContext, reply, this);
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
  if (_publishContext == NULL) return ENOTCONN;
  connected = true;
  _publishContext->privdata = this;
  _subscribeContext->privdata = this;
  return 0;
}

int BrokerRedis::disconnect() {
  LOGI << (" disconnecting.") << LEND;
  if (!connected()) return 0;
  _thread.deleteInvoker(_subscribeContext->fd);
  redisFree(_publishContext);
  redisFree(_subscribeContext);
  _subscribers.clear();
  connected = false;
  return 0;
}

int BrokerRedis::subscribe(string pattern) {
  INFO(" REDIS psubscribe %s", pattern.c_str());
  if (_subscribers.find(pattern) == _subscribers.end()) {
    SubscriberStruct *sub = new SubscriberStruct({pattern});
    string cmd = stringFormat("PSUBSCRIBE %s", pattern.c_str());
    INFO(" REDIS cmd %s", cmd.c_str());
    redisReply *r = (redisReply *)redisCommand(_subscribeContext, cmd.c_str());
    if (r) {
      LOGI << cmd << LEND;
      _subscribers.emplace(pattern, sub);
      freeReplyObject(r);
    } else {
      LOGW << cmd << " failed." << LEND;
    }
  } else {
  }
  return 0;
}

int BrokerRedis::unSubscribe(string pattern) {
  auto it = _subscribers.find(pattern);
  if (it == _subscribers.end()) {
  } else {
    redisReply *r = (redisReply *)redisCommand(
        _subscribeContext, "PUNSUBSCRIBE %s", it->second->pattern);
    if (r) {
      LOGI << " PUNSUBSCRIBE : " << it->second->pattern << " created." << LEND;
      _subscribers.erase(pattern);
      freeReplyObject(r);
    }
  }
  return 0;
}

int BrokerRedis::publish(string topic, Bytes &bs) {
  if (!connected()) return ENOTCONN;

  redisReply *r = (redisReply *)redisCommand(_publishContext, "PUBLISH %s %b",
                                             topic, bs.data(), bs.size());
  if (r == 0) {
    LOGW << "PUBLISH failed " << topic << LEND;
  } else {
    LOGD << " PUBLISH : " << topic << ":" << cborDump(bs) << LEND;
  }
  return 0;
}

SubscriberStruct *BrokerRedis::findSub(string pattern) {
  for (auto it : _subscribers) {
    if (it.second->pattern == pattern) return it.second;
  }
  return 0;
}

int BrokerRedis::command(const char *format, ...) {
  if (!connected()) return ENOTCONN;
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

#include <regex>
bool BrokerRedis::match(string pattern, string topic) {
  std::regex patternRegex(pattern);
  return std::regex_match(topic, patternRegex);
}