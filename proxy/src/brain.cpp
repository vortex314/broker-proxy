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
  BrokerAbstract *broker = new BrokerZenoh(workerThread, brokerConfig);
  // broker.scout();
  broker->init();
  int rc = broker->connect("brain");
  rc = broker->subscriber(1, "src/**", [&](int id,string key, const Bytes &bs) {
    LOGI << "Sub rxd  : " << key << " = " << id << ':'
         << hexDump(bs) << LEND;
  });

  workerThread.run();
    rc = broker->publisher(2, "src/brain/system/uptime");
  //  rc = broker->publish(2, Sys::millis());
  broker->disconnect();
}
