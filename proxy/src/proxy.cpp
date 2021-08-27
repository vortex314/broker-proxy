#include <ArduinoJson.h>
#include <config.h>
#include <log.h>
#include <stdio.h>

#include <thread>
#include <unordered_map>
#include <utility>

using namespace std;

LogS logger;
#ifdef BROKER_ZENOH
#include <BrokerZenoh.h>
#endif
#ifdef BROKER_REDIS
#include <BrokerRedis.h>
#endif
#include <CborDump.h>
#include <Frame.h>
#include <ReflectFromCbor.h>
#include <ReflectToCbor.h>
#include <ReflectToDisplay.h>
#include <SessionSerial.h>
#include <SessionUdp.h>
#include <broker_protocol.h>
const int MsgPublish::TYPE;
const int MsgPublisher::TYPE;
const int MsgSubscriber::TYPE;
const int MsgConnect::TYPE;
const int MsgDisconnect::TYPE;
//====================================================

const char *CMD_TO_STRING[] = {"B_CONNECT",   "B_DISCONNECT", "B_SUBSCRIBER",
                               "B_PUBLISHER", "B_PUBLISH",    "B_RESOURCE",
                               "B_QUERY"};
StaticJsonDocument<10240> doc;

#define fatal(message)                                                         \
  {                                                                            \
    LOGW << message << LEND;                                                   \
    exit(-1);                                                                  \
  }

Config loadConfig(int argc, char **argv) {
  Config cfg = doc.to<JsonObject>();
  // defaults
  cfg["serial"]["port"] = "/dev/ttyUSB0";
  cfg["serial"]["baudrate"] = 115200;
  cfg["broker"]["host"] = "localhost";
  cfg["broker"]["port"] = 6379;
  // override args
  int c;
  while ((c = getopt(argc, argv, "h:p:s:b:")) != -1)
    switch (c) {
    case 'b':
      cfg["serial"]["baudrate"] = atoi(optarg);
      break;
    case 's':
      cfg["serial"]["port"] = optarg;
      break;
    case 'h':
      cfg["broker"]["host"] = optarg;
      break;
    case 'p':
      cfg["broker"]["port"] = atoi(optarg);
      break;
    case '?':
      printf("Usage %s -h <host> -p <port> -s <serial_port> -b <baudrate>\n",
             argv[0]);
      break;
    default:
      WARN("Usage %s -h <host> -p <port> -s <serial_port> -b <baudrate>\n",
           argv[0]);
      abort();
    }

  string sCfg;
  serializeJson(doc, sCfg);
  LOGI << sCfg << LEND;
  return cfg;
};

//================================================================
class MsgFilter : public LambdaFlow<bytes, Bytes> {
  int _msgType;
  MsgBase msgBase;
  ReflectFromCbor _fromCbor;

public:
  MsgFilter(int msgType)
      : LambdaFlow<bytes, Bytes>([this](bytes &out, const bytes &in) {
          //         INFO(" filter on msgType : %d in %s ",
          //         _msgType,cborDump(in).c_str());
          if (msgBase.reflect(_fromCbor.fromBytes(in)).success() &&
              msgBase.msgType == _msgType) {
            //            INFO(" found msgType : %d  ", msgBase.msgType);
            out = in;
            return true;
          }
          return false;
        }),
        _fromCbor(1024) {
    _msgType = msgType;
  };
  static MsgFilter &nw(int msgType) { return *new MsgFilter(msgType); }
};

//==========================================================================
int main(int argc, char **argv) {
  LOGI << "Loading configuration." << LEND;
  Config config = loadConfig(argc, argv);
  Thread workerThread("worker");
  Config serialConfig = config["serial";

  unordered_map<int,string> publishers;

  // SessionSerial session(workerThread, serialConfig);

  SessionAbstract *session;
  if (config["serial"])
    session = new SessionSerial(workerThread, config["serial"]);
  else if (config["udp"])
    session = new SessionUdp(workerThread, config["udp"]);
  else
    fatal(" no interface specified.");
  Config brokerConfig = config["broker"];

#ifdef BROKER_ZENOH
  INFO(" Launching Zenoh");
  BrokerZenoh broker(workerThread, brokerConfig);
#endif
#ifdef BROKER_REDIS
  INFO(" Launching Redis");
  BrokerRedis broker(workerThread, brokerConfig);
#endif
  ReflectFromCbor fromCbor(1024);
  ReflectToCbor toCbor(1024);
  session->init();
  session->connect();
  // zSession.scout();
  broker.init();
  // CBOR de-/serialization

  session->incoming() >>
      [&](const bytes &bs) { INFO("RXD %s", cborDump(bs).c_str()); };


  // filter commands from uC
  session->incoming() >> MsgFilter::nw(B_CONNECT) >> [&](const bytes &frame) {
    MsgConnect msgConnect;
    if (msgConnect.reflect(fromCbor.fromBytes(frame)).success()) {
      int rc = broker.connect(msgConnect.clientId);
    }
  };

  broker.connected() >> [&](const bool &connected) {
    if (connected) {
      MsgConnect msgConnectReply = {"connected"};
      session->outgoing().on(msgConnectReply.reflect(toCbor).toBytes());
    } else {
      MsgDisconnect msgDisconnectReply = {};
      session->outgoing().on(msgDisconnectReply.reflect(toCbor).toBytes());
    }
  };

  session->incoming() >> MsgFilter::nw(B_SUBSCRIBER) >>
      [&](const bytes &frame) {
        MsgSubscriber msgSubscriber;
        if (msgSubscriber.reflect(fromCbor.fromBytes(frame)).success()) {
          int rc = broker.subscriber(
              msgSubscriber.id, msgSubscriber.topic,
              [&](int id, string &, const bytes &bs) {
                MsgPublish msgPublish = {id, bs};
                session->outgoing().on(msgPublish.reflect(toCbor).toBytes());
              });
          if (rc)
            WARN(" subscriber (%s,..) = %d ", msgSubscriber.topic.c_str(), rc);
        }
      };

  session->incoming() >> MsgFilter::nw(B_PUBLISHER) >> [&](const bytes &frame) {
    MsgPublisher msgPublisher;
    if (msgPublisher.reflect(fromCbor.fromBytes(frame)).success()) {
      int rc = broker.publisher(msgPublisher.id, msgPublisher.topic);
      if (rc)
        WARN("  publish (%s,..) = %d ", msgPublisher.topic.c_str(), rc);
    };
  };

  session->incoming() >> MsgFilter::nw(B_PUBLISH) >> [&](const bytes &frame) {
    MsgPublish msgPublish;
    if (msgPublish.reflect(fromCbor.fromBytes(frame)).success()) {
      broker.publish(msgPublish.id, msgPublish.value);
    }
  };

  session->incoming() >> MsgFilter::nw(B_DISCONNECT) >>
      [&](const bytes &frame) {
        MsgDisconnect msgDisconnect;
        if (msgDisconnect.reflect(fromCbor.fromBytes(frame)).success()) {
          broker.disconnect();
        }
      };
  session->connected() >> [&](const bool isConnected) {
    if (!isConnected)
      broker.disconnect();
  };

  workerThread.run();
  delete session;
}
