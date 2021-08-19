#include <CborDump.h>
#include <ReflectFromCbor.h>
#include <ReflectToCbor.h>
#include <ReflectToDisplay.h>
#include <assert.h>
#include <broker_protocol.h>
#include <hiredis.h>
#include <log.h>

// using namespace sw::redis;

#include <iostream>
using namespace std;
const int MsgPublish::TYPE;
const int MsgPublisher::TYPE;
const int MsgSubscriber::TYPE;
const int MsgConnect::TYPE;
const int MsgDisconnect::TYPE;

LogS logger;

#include <gtest/gtest.h>
#define KEY_COUNT 5

#define panicAbort(fmt, ...)                                                   \
  do {                                                                         \
    fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, __LINE__, __func__,          \
            __VA_ARGS__);                                                      \
    exit(-1);                                                                  \
  } while (0)

static void assertReplyAndFree(redisContext *context, redisReply *reply,
                               int type) {
  if (reply == NULL)
    panicAbort("NULL reply from server (error: %s)", context->errstr);

  if (reply->type != type) {
    if (reply->type == REDIS_REPLY_ERROR)
      fprintf(stderr, "Redis Error: %s\n", reply->str);

    panicAbort("Expected reply type %d but got type %d", type, reply->type);
  }

  freeReplyObject(reply);
}

void privdata_dtor(void *privdata) {
  unsigned int *icount = (unsigned int *)privdata;
  printf("privdata_dtor():  In context privdata dtor (invalidations: %u)\n",
         *icount);
}
/* Switch to the RESP3 protocol and enable client tracking */
static void enableClientTracking(redisContext *c) {
  redisReply *reply = (redisReply *)redisCommand(c, "HELLO 3");
  EXPECT_FALSE(reply == NULL || c->err);

  if (reply->type != REDIS_REPLY_MAP) {
    fprintf(stderr, "Error: Can't send HELLO 3 command.  Are you sure you're ");
    fprintf(stderr, "connected to redis-server >= 6.0.0?\nRedis error: %s\n",
            reply->type == REDIS_REPLY_ERROR ? reply->str : "(unknown)");
    exit(-1);
  }

  freeReplyObject(reply);

  /* Enable client tracking */
  reply = (redisReply *)redisCommand(c, "CLIENT TRACKING ON");
  assertReplyAndFree(c, reply, REDIS_REPLY_STATUS);
}
// Demonstrate some basic assertions.
TEST(RedisConnect, BasicAssertions) {
  unsigned int j, invalidations = 0;

  redisContext *c;
  redisReply *reply;
  const char *hostname = "127.0.0.1";
  int port = 6379;

  redisOptions o = {0};
  REDIS_OPTIONS_SET_TCP(&o, hostname, port);
  REDIS_OPTIONS_SET_PRIVDATA(&o, &invalidations, privdata_dtor);
  c = redisConnectWithOptions(&o);
  EXPECT_FALSE(c == NULL || c->err);
  enableClientTracking(c);
  /* Set some keys and then read them back.  Once we do that, Redis will deliver
   * invalidation push messages whenever the key is modified */
  for (j = 0; j < KEY_COUNT; j++) {
    reply = (redisReply *)redisCommand(c, "SET key:%d initial:%d", j, j);
    assertReplyAndFree(c, reply, REDIS_REPLY_STATUS);

    reply = (redisReply *)redisCommand(c, "GET key:%d", j);
    assertReplyAndFree(c, reply, REDIS_REPLY_STRING);
  }

  /* Trigger invalidation messages by updating keys we just read */
  for (j = 0; j < KEY_COUNT; j++) {
    printf("            main(): SET key:%d update:%d\n", j, j);
    reply = (redisReply *)redisCommand(c, "SET key:%d update:%d", j, j);
    assertReplyAndFree(c, reply, REDIS_REPLY_STATUS);
    printf("            main(): SET REPLY OK\n");
  }

  printf("\nTotal detected invalidations: %d, expected: %d\n", invalidations,
         KEY_COUNT);
  redisFree(c);
}
//=============================================================================
#include <adapters/libevent.h>
#include <async.h>

void onMessage(redisAsyncContext *c, void *reply, void *privdata) {
    redisReply *r =(redisReply *) reply;
    if (reply == NULL) return;

    if (r->type == REDIS_REPLY_ARRAY) {
        for (int j = 0; j < r->elements; j++) {
            printf("%u) %s\n", j, r->element[j]->str);
        }
    }
}

void getCallback(redisAsyncContext *c, void *r, void *privdata) {
  redisReply *reply = (redisReply *)r;
  if (reply == NULL)
    return;
  printf("argv[%s]: %s\n", (char *)privdata, reply->str);

  /* Disconnect after receiving the reply to GET */
  redisAsyncDisconnect(c);
}

void connectCallback(const redisAsyncContext *c, int status) {
  if (status != REDIS_OK) {
    printf("Error: %s\n", c->errstr);
    return;
  }
  printf("Connected...\n");
}

void disconnectCallback(const redisAsyncContext *c, int status) {
  if (status != REDIS_OK) {
    printf("Error: %s\n", c->errstr);
    return;
  }
  printf("Disconnected...\n");
}

TEST(RedisConnectAsync, BasicAssertions) {
  signal(SIGPIPE, SIG_IGN);
  struct event_base *base = event_base_new();

  redisAsyncContext *ac = redisAsyncConnect("127.0.0.1", 6379);
  EXPECT_EQ(ac->err, 0);
  int rc ;
  EXPECT_EQ(redisLibeventAttach(ac, base),0);
  EXPECT_EQ(redisAsyncSetConnectCallback(ac, connectCallback),0);
  EXPECT_EQ(redisAsyncSetDisconnectCallback(ac, disconnectCallback),0);
  EXPECT_EQ(redisAsyncCommand(ac, onMessage, NULL, "SUBSCRIBE testtopic"),0);
  EXPECT_EQ(redisAsyncCommand(ac, onMessage, NULL, "SUBSCRIBE anothertopic"),0);
  EXPECT_EQ(redisAsyncCommand(ac, NULL, NULL, "PUBLISH testtopîc %b", "key",
                    strlen("key")),0);
  EXPECT_EQ(redisAsyncCommand(ac, getCallback, (char *)"end-1", "GET key"),0);
  event_base_dispatch(base);
  redisAsyncDisconnect(ac);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}