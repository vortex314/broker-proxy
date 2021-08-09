#include <CborDump.h>
#include <ReflectFromCbor.h>
#include <ReflectToCbor.h>
#include <ReflectToDisplay.h>
#include <assert.h>
#include <broker_protocol.h>
#include <log.h>

#include <iostream>
using namespace std;
const int MsgPublish::TYPE;
const int MsgPublisher::TYPE;
const int MsgSubscriber::TYPE;
const int MsgConnect::TYPE;
const int MsgDisconnect::TYPE;

LogS logger;

#include <gtest/gtest.h>

// Demonstrate some basic assertions.
TEST(HelloTest, BasicAssertions) {
  // Expect two strings not to be equal.
  EXPECT_STRNE("hello", "world");
  // Expect equality.
  EXPECT_EQ(7 * 6, 42);
}

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

TEST(CborSerializer, VerifyU64) {
  uint64_t x = 5863016105212345;
  CborSerializer ser(100);
  CborDeserializer deser(100);
  EXPECT_EQ(ser.begin().add(x).end().success(), true) << " serialize failed";
  Bytes result = ser.toBytes();
  Bytes expected = {0x9F, 0x1B, 0x00, 0x14, 0xD4, 0x61,
                    0xD0, 0x43, 0x79, 0xB9, 0xFF};
  EXPECT_EQ(result, expected);
  uint64_t x2;
  deser.fromBytes(result).begin() >> x2;
  EXPECT_EQ(x, x2);
  char str[100];
  sprintf(str, "%lu", x);
  EXPECT_EQ(strcmp(str, "5863016105212345"), 0);
}

#include <etl/string_stream.h>

TEST(ETL_string_stream, verifyu64) {
  etl::string<100> s1, s2;
  etl::string_stream ss(s1);
  s2 = "5863016105212345";
  ss << 5863016105212345;
  EXPECT_EQ(s1, s2);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}