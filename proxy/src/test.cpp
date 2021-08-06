#include <ReflectToCbor.h>
#include <CborDump.h>
#include <ReflectFromCbor.h>
#include <ReflectToDisplay.h>
#include <log.h>
#include <assert.h>
#include <broker_protocol.h>

#include <iostream>
using namespace std;
const int MsgPublish::TYPE;
const int MsgPublisher::TYPE;
const int MsgSubscriber::TYPE;
const int MsgConnect::TYPE;
const int MsgDisconnect::TYPE;

LogS logger;

#include <gtest/gtest.h>

TEST(MsgPublisher, BasicAssertions) {
  MsgPublisher msgPublisher = {2, "system/uptime"};
  ReflectToCbor toCbor(1024);
  msgPublisher.reflect(toCbor);
  LOGI << " msgPublisher " << hexDump(toCbor.toBytes()) << LEND;
  EXPECT_EQ(toCbor.toBytes().size(), 18);
  byte data[] = {0x9F, 0x03, 0x02, 0x6D, 0x73, 0x79, 0x73, 0x74, 0x65,
                 0x6D, 0x2F, 0x75, 0x70, 0x74, 0x69, 0x6D, 0x65, 0xFF};
  EXPECT_EQ(toCbor.toBytes(), Bytes(data, data + sizeof(data)));
}

TEST(MsgSubscriber, BasicAssertions) {
  MsgSubscriber msgSubscriber = {3, "system/*"};
  ReflectToCbor toCbor(100);
  Bytes bs = msgSubscriber.reflect(toCbor).toBytes();
  INFO("%s", cborDump(bs).c_str());
  ReflectFromCbor fromCbor(100);
  MsgSubscriber msgSubscriber2;
  msgSubscriber2.reflect(fromCbor.fromBytes(bs));
  EXPECT_EQ(msgSubscriber.TYPE, msgSubscriber2.TYPE);
  EXPECT_EQ(msgSubscriber.id, msgSubscriber2.id);
  EXPECT_EQ(msgSubscriber.topic, msgSubscriber2.topic);
}

TEST(MsgPublish, BasicAssertions) {
  String s = "i/am/alive";
  Bytes data = Bytes(s.c_str(), s.c_str() + s.size());
  MsgPublish msgPublish = {3, data};
  ReflectToCbor toCbor(100);
  Bytes bs = msgPublish.reflect(toCbor).toBytes();
  INFO("%s", cborDump(bs).c_str());
  ReflectFromCbor fromCbor(100);
  MsgPublish msgPublish2;
  msgPublish2.reflect(fromCbor.fromBytes(bs));
  EXPECT_EQ(msgPublish.TYPE, msgPublish2.TYPE);
  EXPECT_EQ(msgPublish.id, msgPublish2.id);
  EXPECT_EQ(msgPublish.value, msgPublish2.value);
}

int func(int argc, char **argv) {
  LOGI << "Start " << argv[0] << LEND;
  ReflectToCbor toCbor(1024);
  ReflectToDisplay toDisplay;
  Bytes data = {0x1, 0x2, 0x3};

  MsgPublish msgPublish = {1, data};
  MsgPublisher msgPublisher = {2, "system/uptime"};
  MsgSubscriber msgSubscriber = {3, "system/*"};
  MsgConnect msgConnect = {"src/esp32/"};

  msgConnect.reflect(toCbor);
  LOGI << " msgConnect " << hexDump(toCbor.toBytes()) << LEND;

  msgPublisher.reflect(toCbor);
  LOGI << " msgPublisher " << hexDump(toCbor.toBytes()) << LEND;

  msgSubscriber.reflect(toCbor);
  LOGI << " msgSubscriber " << hexDump(toCbor.toBytes()) << LEND;

  LOGI << " msgPublish " << hexDump(msgPublish.reflect(toCbor).toBytes())
       << LEND;
  LOGI << " msgPublish " << msgPublish.reflect(toDisplay).toString() << LEND;
  return 0;
}