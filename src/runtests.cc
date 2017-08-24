
#include <algorithm>
#include <memory>

#include "gtest/gtest.h"
#include "proto/control.pb.h"
#include "proto/header.pb.h"
#include "proto/testing_api.pb.h"
#include "src/util.h"

using namespace nugget::app::protoapi;

using std::cout;
using std::vector;
using std::unique_ptr;

namespace {

using test_harness::BYTE_TIME;

class NuggetOsTestFixture: public testing::Test {
 protected:
  static void SetUpTestCase();
  static void TearDownTestCase();

 public:
  static unique_ptr<test_harness::TestHarness> harness;
  static std::random_device random_number_generator;
};

unique_ptr<test_harness::TestHarness> NuggetOsTestFixture::harness;
std::random_device NuggetOsTestFixture::random_number_generator;

void NuggetOsTestFixture::SetUpTestCase() {
  harness = unique_ptr<test_harness::TestHarness>(
      new test_harness::TestHarness());
  EXPECT_TRUE(harness->ttyState());
}

void NuggetOsTestFixture::TearDownTestCase() {
  harness->flushUntil(test_harness::BYTE_TIME * 1024);
  harness = unique_ptr<test_harness::TestHarness>();
}

TEST_F(NuggetOsTestFixture, NoticePingTest) {
  harness->flushUntil(test_harness::BYTE_TIME * 1024);

  Notice ping_msg;
  ping_msg.set_notice_code(NoticeCode::PING);
  Notice pong_msg;

  EXPECT_EQ(harness->sendProto(APImessageID::NOTICE, ping_msg), 0);
  cout << ping_msg.DebugString();
  test_harness::raw_message receive_msg;
  EXPECT_EQ(harness->getAhdlc(&receive_msg, 4096 * BYTE_TIME), 0);
  EXPECT_EQ(receive_msg.type, APImessageID::NOTICE);
  pong_msg.set_notice_code(NoticeCode::PING);
  pong_msg.ParseFromArray((char *) receive_msg.data, receive_msg.data_len);
  cout << pong_msg.DebugString() <<std::endl;
  EXPECT_EQ(pong_msg.notice_code(), NoticeCode::PONG);

  EXPECT_EQ(harness->sendProto(APImessageID::NOTICE, ping_msg), 0);
  cout << ping_msg.DebugString();
  EXPECT_EQ(harness->getAhdlc(&receive_msg, 4096 * BYTE_TIME), 0);
  EXPECT_EQ(receive_msg.type, APImessageID::NOTICE);
  pong_msg.set_notice_code(NoticeCode::PING);
  pong_msg.ParseFromArray((char *) receive_msg.data, receive_msg.data_len);
  cout << pong_msg.DebugString() <<std::endl;
  EXPECT_EQ(pong_msg.notice_code(), NoticeCode::PONG);

  EXPECT_EQ(harness->sendProto(APImessageID::NOTICE, ping_msg), 0);
  cout << ping_msg.DebugString();
  EXPECT_EQ(harness->getAhdlc(&receive_msg, 4096 * BYTE_TIME), 0);
  harness->flushUntil(test_harness::BYTE_TIME * 1024);
  EXPECT_EQ(receive_msg.type, APImessageID::NOTICE);
  pong_msg.set_notice_code(NoticeCode::PING);
  pong_msg.ParseFromArray((char *) receive_msg.data, receive_msg.data_len);
  cout << pong_msg.DebugString() <<std::endl;
  EXPECT_EQ(pong_msg.notice_code(), NoticeCode::PONG);
}

TEST_F(NuggetOsTestFixture, InvalidMessageTypeTest) {
  harness->flushUntil(test_harness::BYTE_TIME * 1024);

  const char content[] = "This is a test message.";

  test_harness::raw_message msg;
  msg.type = 0;
  std::copy(content, content + sizeof(content), msg.data);
  msg.data_len = sizeof(content);

  EXPECT_EQ(harness->sendAhdlc(msg), 0);
  EXPECT_EQ(harness->getAhdlc(&msg, 4096 * BYTE_TIME), 0);
  harness->flushUntil(test_harness::BYTE_TIME * 1024);
  EXPECT_EQ(msg.type, APImessageID::NOTICE);

  Notice notice_msg;
  notice_msg.ParseFromArray((char *) msg.data, msg.data_len);
  cout << notice_msg.DebugString() <<std::endl;
  EXPECT_EQ(notice_msg.notice_code(), NoticeCode::UNRECOGNIZED_MESSAGE);

}

TEST_F(NuggetOsTestFixture, AesCbCTest_) {
  harness->flushUntil(test_harness::BYTE_TIME * 1024);

  for (auto key_size : {KeySize::s128b, KeySize::s256b, KeySize::s512b}) {
    cout << "Testing with a key size of: " << std::dec << (key_size * 8)
         <<std::endl;
    AesCbcEncryptTest request;
    request.set_key_size(key_size);
    request.set_number_of_blocks(10);

    vector<int> key_data(key_size * 8 / sizeof(int));
    for (auto &part : key_data) {
      part = random_number_generator();
    }
    request.set_key(key_data.data(), key_data.size() * sizeof(int));

    EXPECT_EQ(harness->sendOneofProto(
        APImessageID::TESTING_API_CALL,
        OneofTestParametersCase::kAesCbcEncryptTest,
        request), 0);

    test_harness::raw_message msg;
    EXPECT_EQ(harness->getAhdlc(&msg, 4096 * BYTE_TIME), 0);
    EXPECT_EQ(msg.type, APImessageID::TESTING_API_RESPONSE);
    EXPECT_GT(msg.data_len, 2);
    uint16_t subtype = (msg.data[0] << 8) | msg.data[1];
    EXPECT_EQ(subtype, OneofTestResultsCase::kAesCbcEncryptTestResult);

    AesCbcEncryptTestResult result;
    result.ParseFromArray((char *) msg.data + 2, msg.data_len - 2);
    EXPECT_EQ(result.result_code(), DcryptError::DE_NO_ERROR);
  }

  harness->flushUntil(test_harness::BYTE_TIME * 1024);
}

}  // namespace
