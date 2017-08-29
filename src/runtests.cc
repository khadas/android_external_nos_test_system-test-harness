
#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>

#include "gtest/gtest.h"
#include "gflags/gflags.h"
#include "openssl/aes.h"
#include "proto/control.pb.h"
#include "proto/header.pb.h"
#include "proto/testing_api.pb.h"
#include "src/util.h"


using namespace nugget::app::protoapi;

using std::cout;
using std::vector;
using std::unique_ptr;

DEFINE_bool(nos_test_dump_protos, false, "Dump binary protobufs to a file.");

#define ASSERT_NO_ERROR(code) \
  ASSERT_EQ(code, test_harness::error_codes::NO_ERROR) \
      << code << " is " << test_harness::error_codes_name(code)

#define ASSERT_MSG_TYPE(msg, type_) \
  ASSERT_EQ(msg.type, type_) \
      << msg.type << " is " << APImessageID_Name((APImessageID) msg.type)

namespace {

using test_harness::BYTE_TIME;

class NuggetOsTest: public testing::Test {
 protected:
  static void SetUpTestCase();
  static void TearDownTestCase();

 public:
  static unique_ptr<test_harness::TestHarness> harness;
  static std::random_device random_number_generator;
};

unique_ptr<test_harness::TestHarness> NuggetOsTest::harness;
std::random_device NuggetOsTest::random_number_generator;

void NuggetOsTest::SetUpTestCase() {
  harness = unique_ptr<test_harness::TestHarness>(
      new test_harness::TestHarness());
  EXPECT_TRUE(harness->ttyState());
}

void NuggetOsTest::TearDownTestCase() {
  harness->flushUntil(test_harness::BYTE_TIME * 1024);
  harness = unique_ptr<test_harness::TestHarness>();
}

TEST_F(NuggetOsTest, NoticePingTest) {
  harness->flushUntil(test_harness::BYTE_TIME * 1024);

  Notice ping_msg;
  ping_msg.set_notice_code(NoticeCode::PING);
  Notice pong_msg;

  ASSERT_NO_ERROR(harness->sendProto(APImessageID::NOTICE, ping_msg));
  cout << ping_msg.DebugString();
  test_harness::raw_message receive_msg;
  ASSERT_NO_ERROR(harness->getAhdlc(&receive_msg, 4096 * BYTE_TIME));
  ASSERT_MSG_TYPE(receive_msg, APImessageID::NOTICE);
  pong_msg.set_notice_code(NoticeCode::PING);
  pong_msg.ParseFromArray((char *) receive_msg.data, receive_msg.data_len);
  cout << pong_msg.DebugString() <<std::endl;
  EXPECT_EQ(pong_msg.notice_code(), NoticeCode::PONG);

  ASSERT_NO_ERROR(harness->sendProto(APImessageID::NOTICE, ping_msg));
  cout << ping_msg.DebugString();
  ASSERT_NO_ERROR(harness->getAhdlc(&receive_msg, 4096 * BYTE_TIME));
  ASSERT_MSG_TYPE(receive_msg, APImessageID::NOTICE);
  pong_msg.set_notice_code(NoticeCode::PING);
  pong_msg.ParseFromArray((char *) receive_msg.data, receive_msg.data_len);
  cout << pong_msg.DebugString() <<std::endl;
  EXPECT_EQ(pong_msg.notice_code(), NoticeCode::PONG);

  ASSERT_NO_ERROR(harness->sendProto(APImessageID::NOTICE, ping_msg));
  cout << ping_msg.DebugString();
  ASSERT_NO_ERROR(harness->getAhdlc(&receive_msg, 4096 * BYTE_TIME));
  harness->flushUntil(test_harness::BYTE_TIME * 1024);
  ASSERT_MSG_TYPE(receive_msg, APImessageID::NOTICE);
  pong_msg.set_notice_code(NoticeCode::PING);
  pong_msg.ParseFromArray((char *) receive_msg.data, receive_msg.data_len);
  cout << pong_msg.DebugString() <<std::endl;
  EXPECT_EQ(pong_msg.notice_code(), NoticeCode::PONG);
}

TEST_F(NuggetOsTest, InvalidMessageTypeTest) {
  harness->flushUntil(test_harness::BYTE_TIME * 1024);

  const char content[] = "This is a test message.";

  test_harness::raw_message msg;
  msg.type = 0;
  std::copy(content, content + sizeof(content), msg.data);
  msg.data_len = sizeof(content);

  ASSERT_NO_ERROR(harness->sendAhdlc(msg));
  ASSERT_NO_ERROR(harness->getAhdlc(&msg, 4096 * BYTE_TIME));
  harness->flushUntil(test_harness::BYTE_TIME * 1024);
  ASSERT_MSG_TYPE(msg, APImessageID::NOTICE);

  Notice notice_msg;
  notice_msg.ParseFromArray((char *) msg.data, msg.data_len);
  cout << notice_msg.DebugString() <<std::endl;
  EXPECT_EQ(notice_msg.notice_code(), NoticeCode::UNRECOGNIZED_MESSAGE);

}

TEST_F(NuggetOsTest, SequenceTest) {
  harness->flushUntil(test_harness::BYTE_TIME * 1024);

  test_harness::raw_message msg;
  msg.type = APImessageID::SEND_SEQUENCE;
  msg.data_len = 256;
  for (size_t x = 0; x < msg.data_len; ++x) {
    msg.data[x] = x;
  }

  ASSERT_NO_ERROR(harness->sendAhdlc(msg));
  ASSERT_NO_ERROR(harness->getAhdlc(&msg, 4096 * BYTE_TIME));
  harness->flushUntil(test_harness::BYTE_TIME * 1024);
  ASSERT_MSG_TYPE(msg, APImessageID::SEND_SEQUENCE);
  for (size_t x = 0; x < msg.data_len; ++x) {
    ASSERT_EQ(msg.data[x], x) << "Inconsistency at index " << x;
  }
}

TEST_F(NuggetOsTest, EchoTest) {
  harness->flushUntil(test_harness::BYTE_TIME * 1024);

  test_harness::raw_message msg;
  msg.type = APImessageID::ECHO_THIS;
  // Leave some room for bytes which need escaping
  msg.data_len = sizeof(msg.data) - 64;
  for (size_t x = 0; x < msg.data_len; ++x) {
    msg.data[x] = random_number_generator();
  }

  ASSERT_NO_ERROR(harness->sendAhdlc(msg));

  test_harness::raw_message receive_msg;
  ASSERT_NO_ERROR(harness->getAhdlc(&receive_msg, 4096 * BYTE_TIME));
  ASSERT_MSG_TYPE(msg, APImessageID::ECHO_THIS);
  ASSERT_EQ(receive_msg.data_len, msg.data_len);

  for (size_t x = 0; x < msg.data_len; ++x) {
    ASSERT_EQ(msg.data[x], receive_msg.data[x])
        << "Inconsistency at index " << x;
  }

  harness->flushUntil(test_harness::BYTE_TIME * 1024);
}

TEST_F(NuggetOsTest, AesCbCTest) {
  harness->flushUntil(test_harness::BYTE_TIME * 1024);
  const size_t number_of_blocks = 3;

  for (auto key_size : {KeySize::s128b, KeySize::s256b, KeySize::s512b}) {
    cout << "Testing with a key size of: " << std::dec << (key_size * 8)
         <<std::endl;
    AesCbcEncryptTest request;
    request.set_key_size(key_size);
    request.set_number_of_blocks(number_of_blocks);

    vector<int> key_data(key_size / sizeof(int));
    for (auto &part : key_data) {
      part = random_number_generator();
    }
    request.set_key(key_data.data(), key_data.size() * sizeof(int));


    if (FLAGS_nos_test_dump_protos) {
      std::ofstream outfile;
      outfile.open("AesCbcEncryptTest_" + std::to_string(key_size * 8) +
                   ".proto.bin", std::ios_base::binary);
      outfile << request.SerializeAsString();
      outfile.close();
    }

    ASSERT_NO_ERROR(harness->sendOneofProto(
        APImessageID::TESTING_API_CALL,
        OneofTestParametersCase::kAesCbcEncryptTest,
        request));

    test_harness::raw_message msg;
    ASSERT_NO_ERROR(harness->getAhdlc(&msg, 4096 * BYTE_TIME));
    ASSERT_MSG_TYPE(msg, APImessageID::TESTING_API_RESPONSE);
    EXPECT_GT(msg.data_len, 2);
    uint16_t subtype = (msg.data[0] << 8) | msg.data[1];
    EXPECT_EQ(subtype, OneofTestResultsCase::kAesCbcEncryptTestResult);

    AesCbcEncryptTestResult result;
    result.ParseFromArray((char *) msg.data + 2, msg.data_len - 2);
    EXPECT_EQ(result.result_code(), DcryptError::DE_NO_ERROR)
        << result.result_code() << " is "
        << DcryptError_Name(result.result_code());
    ASSERT_EQ(result.cipher_text().size(), number_of_blocks * AES_BLOCK_SIZE)
        << "\n" << result.DebugString();

    uint32_t in[4] = {0, 0, 0, 0};
    uint8_t sw_out[AES_BLOCK_SIZE];
    uint8_t iv[AES_BLOCK_SIZE];
    memset(&iv, 0, sizeof(iv));
    AES_KEY aes_key;
    AES_set_encrypt_key((uint8_t *) key_data.data(), key_size * 8, &aes_key);
    for (size_t x = 0; x < number_of_blocks; ++x) {
      AES_cbc_encrypt((uint8_t *) in, (uint8_t *) sw_out, AES_BLOCK_SIZE,
                      &aes_key, (unsigned char *) iv, true);
      for (size_t y = 0; y < AES_BLOCK_SIZE; ++y) {
        size_t index = x * AES_BLOCK_SIZE + y;
        ASSERT_EQ(result.cipher_text()[index] & 0x00ff,
                  sw_out[y] & 0x00ff) << "Inconsistency at index " << index;
      }
    }

    ASSERT_EQ(result.initialization_vector().size(), AES_BLOCK_SIZE)
        << "\n" << result.DebugString();
    for (size_t x = 0; x < AES_BLOCK_SIZE; ++x) {
      ASSERT_EQ(result.initialization_vector()[x] & 0x00ff, iv[x] & 0x00ff)
                << "Inconsistency at index " << x;
    }
  }

  harness->flushUntil(test_harness::BYTE_TIME * 1024);
}

}  // namespace
