#include <fstream>
#include <iostream>
#include <sstream>

#include "gtest/gtest.h"
#include "gflags/gflags.h"
#include "protoapi/control.pb.h"
#include "protoapi/header.pb.h"
#include "protoapi/testing_api.pb.h"
#include "src/util.h"


using nugget::app::protoapi::AesGcmEncryptTest;
using nugget::app::protoapi::AesGcmEncryptTestResult;
using nugget::app::protoapi::DcryptError;
using nugget::app::protoapi::Notice;
using nugget::app::protoapi::NoticeCode;
using nugget::app::protoapi::OneofTestParametersCase;
using nugget::app::protoapi::OneofTestResultsCase;
using std::cout;
using std::stringstream;
using std::unique_ptr;

DEFINE_bool(nos_test_dump_protos, false, "Dump binary protobufs to a file.");

#define ASSERT_NO_ERROR(code) \
  ASSERT_EQ(code, test_harness::error_codes::NO_ERROR) \
      << code << " is " << test_harness::error_codes_name(code)

#define ASSERT_MSG_TYPE(msg, type_) \
do{if(type_ != APImessageID::NOTICE && msg.type == APImessageID::NOTICE){ \
  Notice received; \
  received.ParseFromArray(reinterpret_cast<char *>(msg.data), msg.data_len); \
  ASSERT_EQ(msg.type, type_) \
      << msg.type << " is " << APImessageID_Name((APImessageID) msg.type) \
      << "\n" << received.DebugString(); \
}else{ \
  ASSERT_EQ(msg.type, type_) \
      << msg.type << " is " << APImessageID_Name((APImessageID) msg.type); \
}}while(0)

#define ASSERT_SUBTYPE(msg, type_) \
  EXPECT_GT(msg.data_len, 2); \
  uint16_t subtype = (msg.data[0] << 8) | msg.data[1]; \
  EXPECT_EQ(subtype, type_)

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

  EXPECT_TRUE(harness->switchFromConsoleToProtoApi());
  EXPECT_TRUE(harness->ttyState());
}

void NuggetOsTest::TearDownTestCase() {
  harness->readUntil(test_harness::BYTE_TIME * 1024);
  EXPECT_TRUE(harness->switchFromProtoApiToConsole(NULL));
  harness = unique_ptr<test_harness::TestHarness>();
}

#include "test-data/NIST-CAVP/aes-gcm-cavp.h"
#define ARRAYSIZE(a) \
  ((sizeof(a) / sizeof(*(a))) / \
   static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))

TEST_F(NuggetOsTest, AesGcm) {
  const int verbosity = harness->getVerbosity();
  harness->setVerbosity(verbosity - 1);
  harness->readUntil(test_harness::BYTE_TIME * 1024);

  for (size_t i = 0; i < ARRAYSIZE(NIST_GCM_DATA); i++) {
    const gcm_data *test_case = &NIST_GCM_DATA[i];

    AesGcmEncryptTest request;
    request.set_key(test_case->key, test_case->key_len / 8);
    request.set_iv(test_case->IV, test_case->IV_len / 8);
    request.set_plain_text(test_case->PT, test_case->PT_len / 8);
    request.set_aad(test_case->AAD, test_case->AAD_len / 8);
    request.set_tag_len(test_case->tag_len / 8);

    if (FLAGS_nos_test_dump_protos) {
      std::ofstream outfile;
      outfile.open("AesGcmEncryptTest_" + std::to_string(test_case->key_len) +
                   ".proto.bin", std::ios_base::binary);
      outfile << request.SerializeAsString();
      outfile.close();
    }

    ASSERT_NO_ERROR(harness->sendOneofProto(
        APImessageID::TESTING_API_CALL,
        OneofTestParametersCase::kAesGcmEncryptTest,
        request));

    test_harness::raw_message msg;
    ASSERT_NO_ERROR(harness->getAhdlc(&msg, 4096 * BYTE_TIME));
    ASSERT_MSG_TYPE(msg, APImessageID::TESTING_API_RESPONSE);
    ASSERT_SUBTYPE(msg, OneofTestResultsCase::kAesGcmEncryptTestResult);

    AesGcmEncryptTestResult result;
    ASSERT_TRUE(result.ParseFromArray(reinterpret_cast<char *>(msg.data + 2),
                                      msg.data_len - 2));
    EXPECT_EQ(result.result_code(), DcryptError::DE_NO_ERROR)
        << result.result_code() << " is "
        << DcryptError_Name(result.result_code());

    ASSERT_EQ(result.cipher_text().size(), test_case->PT_len / 8)
            << "\n" << result.DebugString();
    const uint8_t *CT = (const uint8_t *)test_case->CT;
    stringstream ss;
    for (size_t j = 0; j < test_case->PT_len / 8; j++) {
      if (CT[j] < 16) {
        ss << '0';
      }
      ss << std::hex << (unsigned int)CT[j];
    }
    for (size_t j = 0; j < test_case->PT_len / 8; j++) {
      ASSERT_EQ(result.cipher_text()[j] & 0x00FF, CT[j] & 0x00FF)
              << "\n"
              << "test_case: " << i << "\n"
              << "result   : " << result.DebugString()
              << "CT       : " << ss.str() << "\n"
              << "mis-match: " << j;
    }

    ASSERT_EQ(result.tag().size(), test_case->tag_len / 8)
            << "\n" << result.DebugString();
    const uint8_t *tag = (const uint8_t *)test_case->tag;
    for (size_t j = 0; j < test_case->tag_len / 8; j++) {
      ASSERT_EQ(result.tag()[j] & 0x00ff, tag[j] & 0x00ff);
    }
  }

  harness->readUntil(test_harness::BYTE_TIME * 1024);
  harness->setVerbosity(verbosity);
}

}  // namespace
