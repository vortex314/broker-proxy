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
#include <CborDeserializer.h>
#include <CborDump.h>
#include <CborSerializer.h>
StaticJsonDocument<10240> doc;

Config loadConfig(int argc, char **argv) {
  Config cfg = doc.to<JsonObject>();
  if (argc > 1)
    cfg["serial"]["port"] = argv[1];
  if (argc > 2)
    cfg["serial"]["baudrate"] = atoi(argv[2]);
  string sCfg;
  serializeJson(doc, sCfg);
  INFO(" config : %s ", sCfg.c_str());
  return cfg;
};

//==========================================================================
int main(int argc, char **argv) {
  Config config = loadConfig(argc, argv);
  Thread workerThread("worker");
  Config brokerConfig = config["zenoh"];
  TimerSource pubTimer(workerThread, 2000, true, "pubTimer");
  CborSerializer cborSerializer(1024);
  CborDeserializer cborDeserializer(1024);
  BrokerAbstract *broker = new BrokerZenoh(workerThread, brokerConfig);
  // broker.scout();
  broker->init();
  int rc = broker->connect("brain");
  rc =
      broker->subscriber(1, "src/**", [&](int id, string key, const Bytes &bs) {
        LOGI << "Sub rxd  : " << key << " = " << id << ':' << cborDump(bs)
             << LEND;
        if (key == "src/brain/system/uptime") {
          uint64_t ts;
          cborDeserializer.fromBytes(bs).begin() >> ts;
          if ( cborDeserializer.success() )
          LOGI << " latency : " << (Sys::micros() - ts) << LEND;
        }
      });
  rc = broker->publisher(2, "src/brain/system/uptime");
  rc = broker->publisher(3, "src/brain/system/latency");

  pubTimer >> [&](const TimerMsg &) {
    LOGI << " Sys::micros() " << Sys::micros() << LEND;
    Bytes bs = (cborSerializer.begin() << Sys::micros()).end().toBytes();
    broker->publish(2, bs);
  };
  workerThread.run();
  broker->disconnect();
}
