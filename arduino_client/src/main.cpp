#include <context.h>

#include "BrokerSerial.h"
#include "Button.h"
#include "LedBlinker.h"

LogS logger;
Thread mainThread("main");
LedBlinker ledBlinkerBlue(mainThread, PIN_LED, 100);
TimerSource aliveTimer(mainThread, 1000, true, "alive");
Button button1(mainThread, PIN_BUTTON);
Poller poller(mainThread);
BrokerSerial brkr(mainThread, Serial);
LambdaSource<uint64_t> systemUptime([]() { return Sys::millis(); });
LambdaSource<String> systemHostname([]() { return Sys::hostname(); });
LambdaSource<const char *> systemBoard([]() { return Sys::board(); });
LambdaSource<const char *> systemCpu([]() { return Sys::cpu(); });
LambdaSource<uint32_t> systemHeap([]() {
#ifdef __ESP32_ARDUINO__
  return ESP.getFreeHeap();
#else
  return 0;
#endif
});

ValueSource<String> systemBuild = String(__DATE__ " " __TIME__);
void serialEvent() { BrokerSerial::onRxd(&brkr); }

void setup() {
  Serial.begin(BAUDRATE);
  aliveTimer >> [](const TimerMsg &) {
    LOGI << Sys::hostname() << " alive."
         << (brkr.connected() ? "connected" : "disconnected") << LEND;
    if (brkr.connected()) {
      systemBuild.request();
      systemHostname.request();
      systemHeap.request();
    }
  };
  Sys::hostname(S(HOSTNAME));

  button1.init();
  ledBlinkerBlue.init();
  brkr.node(Sys::hostname());
  brkr.init();
  brkr.connected >> ledBlinkerBlue.blinkSlow;
  systemBuild >> brkr.publisher<String>("system/build");
  systemHostname >> brkr.publisher<String>("system/hostname");
  systemHeap >> brkr.publisher<uint32_t>("system/heap");
}

void loop() {
  uint64_t startTime = Sys::millis();
  mainThread.loop();
  if (Serial.available()) {
    BrokerSerial::onRxd(&brkr);
  }
  uint64_t delay = (Sys::millis() - startTime);
  if (delay > 10)
    LOGI << " delay : " << delay << LEND;
}