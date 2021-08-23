#include <ArduinoJson.h>
#include <config.h>
#include <log.h>
#include <stdio.h>

#include <thread>
#include <unordered_map>
#include <utility>

using namespace std;
std::vector<std::string> split(const std::string &s, char seperator);

LogS logger;
#ifdef BROKER_ZENOH
#include <BrokerZenoh.h>
#endif
#ifdef BROKER_REDIS
#include <BrokerRedis.h>
#endif
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
#ifdef BROKER_ZENOH
  BrokerZenoh broker(workerThread, brokerConfig);
  broker.init();
  int rc = broker.connect("brain");
  rc = broker.subscriber(1, "src/**", [&](int id, string key, const Bytes &bs) {
#endif
#ifdef BROKER_REDIS
    BrokerRedis broker(workerThread, brokerConfig);
    broker.init();
    int rc = broker.connect("brain");
    rc =
        broker.subscriber(1, "src/*", [&](int id, string key, const Bytes &bs) {
#endif
          // broker.scout();
          //          LOGI << key << "=" << cborDump(bs) << LEND;
          if (key == "src/brain/system/uptime") {
            uint64_t ts;
            uint64_t delay;
            cborDeserializer.fromBytes(bs).begin() >> ts;
            delay = Sys::micros() - ts;
            LOGI << " recv ts " << ts << LEND;

            if (cborDeserializer.success())
              LOGI << " latency : " << delay << " usec " << LEND;
          }
        });
    rc = broker.publisher(2, "src/brain/system/uptime");
    rc = broker.publisher(3, "src/brain/system/latency");
#ifdef BROKER_REDIS
    broker.subscriber(2, "src/*", [&](int id, string key, const Bytes &bs) {
      //    broker.command(stringFormat("SET %s \%b", key.c_str()).c_str(),
      //    bs.data(), bs.size());
      vector<string> parts = split(key, '/');
      int64_t i64;
      if (cborDeserializer.fromBytes(bs).begin().get(i64).success()) {
        broker.command(stringFormat("SET %s %ld ", key.c_str(), i64).c_str());
        broker.command(
            stringFormat(
                "ts.add ts-%s %lu %ld LABELS node %s object %s prop %s ",key.c_str(),
                Sys::millis() / 1000, i64, parts[1].c_str(), parts[2].c_str(),
                parts[3].c_str())
                .c_str());
        broker.command(stringFormat("XADD %s * %s %ld", parts[1].c_str(),
                                    (parts[2] + "/" + parts[3]).c_str(), i64)
                           .c_str());
      }
      double d;
      if (cborDeserializer.fromBytes(bs).begin().get(d).success()) {
        broker.command(stringFormat("SET %s %f ", key.c_str(), d).c_str());
        broker.command(
            stringFormat(
                "ts.add ts-uc %lu %f LABELS node %s object %s prop %s ",
                Sys::millis() / 1000, d, parts[1].c_str(), parts[2].c_str(),
                parts[3].c_str())
                .c_str());
        broker.command(stringFormat("XADD %s * %s %f", parts[1].c_str(),
                                    (parts[2] + "/" + parts[3]).c_str(), d)
                           .c_str());
      }
      string s;
      if (cborDeserializer.fromBytes(bs).begin().get(s).success())
        broker.command(
            stringFormat("SET  %s \"%s\" ", key.c_str(), s.c_str()).c_str());
    });
#endif

    pubTimer >> [&](const TimerMsg &) {
      Bytes bs = cborSerializer.begin().add(Sys::micros()).end().toBytes();
      broker.publish(2, bs);
    };
    workerThread.run();
    broker.disconnect();
}


std::vector<std::string> split(const std::string &s, char seperator) {
    std::vector<std::string> output;

    std::string::size_type prev_pos = 0, pos = 0;

    while ((pos = s.find(seperator, pos)) != std::string::npos) {
      std::string substring(s.substr(prev_pos, pos - prev_pos));

      output.push_back(substring);

      prev_pos = ++pos;
    }

    output.push_back(s.substr(prev_pos, pos - prev_pos)); // Last word

    return output;
}