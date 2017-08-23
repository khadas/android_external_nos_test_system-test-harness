
#include <algorithm>
#include <memory>

#include "gtest/gtest.h"
#include "proto/control.pb.h"
#include "proto/header.pb.h"
#include "src/util.h"

using nugget::app::protoapi::Notice;
using nugget::app::protoapi::NoticeCode;
using nugget::app::protoapi::Notice;

namespace {

using test_harness::BYTE_TIME;

class NuggetOsTestFixture: public testing::Test {
 protected:
  static void SetUpTestCase();
  static void TearDownTestCase();

 public:
  static std::unique_ptr<test_harness::TestHarness> harness;
};

std::unique_ptr<test_harness::TestHarness> NuggetOsTestFixture::harness;

void NuggetOsTestFixture::SetUpTestCase() {
  harness = std::unique_ptr<test_harness::TestHarness>(
      new test_harness::TestHarness());
  EXPECT_TRUE(harness->ttyState());
}

void NuggetOsTestFixture::TearDownTestCase() {
  harness->flushUntil(test_harness::BYTE_TIME * 1024);
  harness = std::unique_ptr<test_harness::TestHarness>();
}

TEST_F(NuggetOsTestFixture, NoticePingTest) {
  harness->flushUntil(test_harness::BYTE_TIME * 1024);

  test_harness::raw_message send_msg;
  send_msg.type = APImessageID::NOTICE;
  Notice notice_msg;
  notice_msg.set_notice_code(NoticeCode::PING);

  int msg_size = notice_msg.ByteSize();
  EXPECT_LT(msg_size, test_harness::PROTO_BUFFER_MAX_LEN);
  send_msg.data_len = (uint16_t) msg_size;
  notice_msg.SerializeToArray(send_msg.data, send_msg.data_len);

  EXPECT_EQ(harness->sendAhdlc(send_msg), 0);
  test_harness::raw_message receive_msg;
  EXPECT_EQ(harness->getAhdlc(&receive_msg, 4096 * BYTE_TIME), 0);
  EXPECT_EQ(receive_msg.type, APImessageID::NOTICE);
  notice_msg.set_notice_code(NoticeCode::PING);
  notice_msg.ParseFromArray((char *) receive_msg.data, receive_msg.data_len);
  std::cout << notice_msg.DebugString() <<std::endl;
  EXPECT_EQ(notice_msg.notice_code(), NoticeCode::PONG);
  harness->flushUntil(test_harness::BYTE_TIME * 1024);

  EXPECT_EQ(harness->sendAhdlc(send_msg), 0);
  EXPECT_EQ(harness->getAhdlc(&receive_msg, 4096 * BYTE_TIME), 0);
  EXPECT_EQ(receive_msg.type, APImessageID::NOTICE);
  notice_msg.set_notice_code(NoticeCode::PING);
  notice_msg.ParseFromArray((char *) receive_msg.data, receive_msg.data_len);
  std::cout << notice_msg.DebugString() <<std::endl;
  EXPECT_EQ(notice_msg.notice_code(), NoticeCode::PONG);
  harness->flushUntil(test_harness::BYTE_TIME * 1024);

  EXPECT_EQ(harness->sendAhdlc(send_msg), 0);
  EXPECT_EQ(harness->getAhdlc(&receive_msg, 4096 * BYTE_TIME), 0);
  EXPECT_EQ(receive_msg.type, APImessageID::NOTICE);
  notice_msg.set_notice_code(NoticeCode::PING);
  notice_msg.ParseFromArray((char *) receive_msg.data, receive_msg.data_len);
  std::cout << notice_msg.DebugString() <<std::endl;
  EXPECT_EQ(notice_msg.notice_code(), NoticeCode::PONG);
  harness->flushUntil(test_harness::BYTE_TIME * 1024);
}

TEST_F(NuggetOsTestFixture, SendRaw) {
  harness->flushUntil(test_harness::BYTE_TIME * 1024);

  const char content[] = "This is a test message.";

  test_harness::raw_message msg;
  msg.type = 0;
  std::copy(content, content + sizeof(content), msg.data);
  msg.data_len = sizeof(content);

  EXPECT_EQ(harness->sendAhdlc(msg), 0);
  EXPECT_EQ(harness->getAhdlc(&msg, 4096 * BYTE_TIME), 0);
  EXPECT_EQ(msg.type, APImessageID::NOTICE);
  Notice notice_msg;
  notice_msg.ParseFromArray((char *) msg.data, msg.data_len);
  std::cout << notice_msg.DebugString() <<std::endl;
  EXPECT_EQ(notice_msg.notice_code(), NoticeCode::UNRECOGNIZED_MESSAGE);

  harness->flushUntil(test_harness::BYTE_TIME * 1024);
}

}  // namespace
