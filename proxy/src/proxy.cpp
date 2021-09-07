#include <ArduinoJson.h>
#include <config.h>
#include <log.h>
#include <stdio.h>
#include <util.h>

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

//====================================================

const char *CMD_TO_STRING[] = {"B_CONNECT",   "B_DISCONNECT", "B_SUBSCRIBER",
                               "B_PUBLISHER", "B_PUBLISH",    "B_RESOURCE",
                               "B_QUERY"};
StaticJsonDocument<10240> doc;

#define fatal(message)       \
  {                          \
    LOGW << message << LEND; \
    exit(-1);                \
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
  while ((c = getopt(argc, argv, "h:p:s:b:")) != -1) switch (c) {
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

//==========================================================================
int main(int argc, char **argv) {
  LOGI << "Loading configuration." << LEND;
  Config config = loadConfig(argc, argv);
  Thread workerThread("worker");
  Config serialConfig = config["serial"];

  string dstPrefix;
  string srcPrefix;

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
  BrokerZenoh brokerProxy(workerThread, brokerConfig);
#endif
#ifdef BROKER_REDIS
  INFO(" Launching Redis");
  BrokerRedis broker(workerThread, brokerConfig);
  BrokerRedis brokerProxy(workerThread, brokerConfig);
#endif
  CborDeserializer fromCbor(1024);
  CborSerializer toCbor(1024);
  session->init();
  session->connect();
  // zSession.scout();
  broker.init();
  broker.connect("serial");
  brokerProxy.init();
  brokerProxy.connect(config["serial"]["port"]);
  // CBOR de-/serialization

  session->incoming() >>
      [&](const bytes &bs) { INFO("RXD %s", cborDump(bs).c_str()); };

  // filter commands from uC
  auto getPubMsg =
      new LambdaFlow<Bytes, PubMsg>([&](PubMsg &msg, const Bytes &frame) {
        int msgType;
        return fromCbor.fromBytes(frame)
                   .begin()
                   .get(msgType)
                   .get(msg.topic)
                   .get(msg.payload)
                   .end()
                   .success() &&
               msgType == B_PUBLISH;
      });

  auto getSubMsg =
      new LambdaFlow<Bytes, SubMsg>([&](SubMsg &msg, const Bytes &frame) {
        int msgType;
        return fromCbor.fromBytes(frame)
                   .begin()
                   .get(msgType)
                   .get(msg.pattern)
                   .success() &&
               msgType == B_SUBSCRIBE;
      });

  session->incoming() >> getPubMsg >> [&](const PubMsg &msg) {
    INFO("PUBLISH %s %s ", msg.topic.c_str(), cborDump(msg.payload).c_str());
    broker.publish(msg.topic, msg.payload);
  };
  session->incoming() >> getSubMsg >>
      [&](const SubMsg &msg) { broker.subscribe(msg.pattern); };

  /* getPubMsg >> [&](const PubMsg &msg) {
     INFO("PUBLISH %s %s ", msg.topic, cborDump(msg.payload).c_str());
   };*/

  broker.incoming() >>
      new LambdaFlow<PubMsg, Bytes>([&](Bytes &bs, const PubMsg &msg) {
        bs = toCbor.begin()
                 .add(MsgPublish::TYPE)
                 .add(msg.topic)
                 .add(msg.payload)
                 .end()
                 .toBytes();
        return toCbor.success();
      }) >>
      session->outgoing();

  workerThread.run();
  delete session;
}
