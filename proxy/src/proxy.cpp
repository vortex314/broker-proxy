#include <ArduinoJson.h>
#include <config.h>
#include <log.h>
#include <stdio.h>

#include <thread>
#include <unordered_map>
#include <utility>

using namespace std;

LogS logger;
#include <BrokerZenoh.h>
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
  if (argc > 1) {
    if (strcmp(argv[1], "udp") == 0) {
      if (argc > 2) {
        int x = atoi(argv[2]);
        cfg["udp"]["port"] = atoi(argv[2]);
      } else
        fatal("missing arguments for udp ");
    } else if (strcmp(argv[1], "serial") == 0) {
      if (argc > 2) {
        cfg["serial"]["port"] = argv[2];
        cfg["serial"]["baudrate"] = 115200;
        if (argc > 3)
          cfg["serial"]["baudrate"] = atoi(argv[3]);
      } else
        fatal(" missing arguments for serial ");
    }
  } else {
    fatal(" insufficient arguments ");
  }
  string sCfg;
  serializeJson(doc, sCfg);
  INFO(" config : %s ", sCfg.c_str());
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
  Config config = loadConfig(argc, argv);
  Thread workerThread("worker");

  Config serialConfig = config["serial"];

  // SessionSerial session(workerThread, serialConfig);

  SessionAbstract *session;
  if (config["serial"])
    session = new SessionSerial(workerThread, config["serial"]);
  else if (config["udp"])
    session = new SessionUdp(workerThread, config["udp"]);
  else
    fatal(" no interface specified.");

  Config brokerConfig = config["zenoh"];
  BrokerZenoh broker(workerThread, brokerConfig);
  ReflectFromCbor fromCbor(1024);
  ReflectToCbor toCbor(1024);
  CHECK;
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
      MsgConnect msgConnectReply = {"connected"};
      session->outgoing().on(msgConnectReply.reflect(toCbor).toBytes());
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
  /*
    frameToCbor >> CborFilter::nw(B_RESOURCE) >> [&](const cbor &param) {
      INFO("Z_RESOURCE");
      string resource = param.to_array()[1];
      zenoh::ResourceKey key = zSession.resource(resource);
      frameToBytes.on(cbor::array{Z_RESOURCE, resource, key});
    };*/
  /*
    frameToCbor >> CborFilter::nw(Z_QUERY) >> [&](const cbor &param) {
      INFO("Z_RESOURCE");
      string uri = param.to_array()[1];
      auto result = zSession.query(uri);
      for (auto res : result) {
        frameToBytes.on(cbor::array{Z_QUERY, res.key, res.data});
      }
    };*/

  session->connected() >> [&](const bool isConnected) {
    if (!isConnected)
      broker.disconnect();
  };

  workerThread.run();
  delete session;
}
