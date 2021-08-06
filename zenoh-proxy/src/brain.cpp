#include <ArduinoJson.h>
#include <log.h>
#include <config.h>
#include <stdio.h>

#include <thread>
#include <unordered_map>
#include <utility>

using namespace std;

LogS logger;
#include <BrokerZenoh.h>
#include <CborDump.h>

//==========================================================================
int main(int argc, char **argv)
{
  Config config = loadConfig(argc, argv);
  Thread workerThread("worker");
  Config brokerConfig = config["zenoh"];
  BrokerZenoh broker(workerThread, brokerConfig);
  broker.scout();
  broker.init();
  int rc = broker.connect("brain");
  rc = broker.subscriber(
      1, "src/brain/system/uptime", [&](int id, const Bytes &bs)
      { LOGI << " Subscriber received : " << id << ':' << string(bs.data(), bs.data() + bs.size()) << LEND; });

  rc = broker.publisher(2, "src/brain/system/uptime");
  rc = broker.publish(2, Sys::millis());
  broker.disconnect();
  workerThread.run();
}
