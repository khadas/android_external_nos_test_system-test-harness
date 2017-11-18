#include <fstream>
#include <iostream>
#include <random>
#include <sstream>

#include "gtest/gtest.h"
#include "gflags/gflags.h"
#include "nugget/app/protoapi/control.pb.h"
#include "nugget/app/protoapi/header.pb.h"
#include "nugget/app/protoapi/testing_api.pb.h"
#include "src/macros.h"
#include "src/util.h"

using nugget::app::protoapi::Asn1TagTest;
using nugget::app::protoapi::Asn1TagTestResult;
using nugget::app::protoapi::APImessageID;
using nugget::app::protoapi::DcryptError;
using nugget::app::protoapi::Notice;
using nugget::app::protoapi::NoticeCode;
using nugget::app::protoapi::OneofTestParametersCase;
using nugget::app::protoapi::OneofTestResultsCase;
using std::cout;
using std::stringstream;
using std::unique_ptr;

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

class Asn1Test: public testing::Test {
 protected:
  static void SetUpTestCase();
  static void TearDownTestCase();

 public:
  static unique_ptr<test_harness::TestHarness> harness;
  static std::random_device random_number_generator;
};

unique_ptr<test_harness::TestHarness> Asn1Test::harness;
std::random_device Asn1Test::random_number_generator;

void Asn1Test::SetUpTestCase() {
  harness = unique_ptr<test_harness::TestHarness>(
      new test_harness::TestHarness());

  if (!harness->UsingSpi()) {
    EXPECT_TRUE(harness->SwitchFromConsoleToProtoApi());
    EXPECT_TRUE(harness->ttyState());
  }
}

void Asn1Test::TearDownTestCase() {
  harness->ReadUntil(test_harness::BYTE_TIME * 1024);
  if (!harness->UsingSpi()) {
    EXPECT_TRUE(harness->SwitchFromProtoApiToConsole(NULL));
  }
  harness = unique_ptr<test_harness::TestHarness>();
}

#include "src/test-data/dcrypto/asn1-data.h"

TEST_F(Asn1Test, ParseTagSuccessTest) {
  const int verbosity = harness->getVerbosity();
  harness->setVerbosity(verbosity - 1);
  harness->ReadUntil(test_harness::BYTE_TIME * 1024);

  for (size_t i = 0; i < ARRAYSIZE(ASN1_VALID_TAGS); i++) {
    const asn1_tag_data *test_case = &ASN1_VALID_TAGS[i];

    Asn1TagTest request;
    request.set_tag(test_case->tag, test_case->tag_len);

    ASSERT_NO_ERROR(harness->SendOneofProto(
        APImessageID::TESTING_API_CALL,
        OneofTestParametersCase::kAsn1TagTest,
        request));

    test_harness::raw_message msg;
    ASSERT_NO_ERROR(harness->GetData(&msg, 4096 * BYTE_TIME));
    ASSERT_MSG_TYPE(msg, APImessageID::TESTING_API_RESPONSE);
    ASSERT_SUBTYPE(msg, OneofTestResultsCase::kAsn1TagTestResult);

    Asn1TagTestResult result;
    ASSERT_TRUE(result.ParseFromArray(reinterpret_cast<char *>(msg.data + 2),
                                      msg.data_len - 2));
    EXPECT_EQ(result.result_code(), DcryptError::DE_NO_ERROR)
      << result.result_code() << " is "
      << DcryptError_Name(result.result_code());

    EXPECT_EQ(result.tag_class(), test_case->tag_class);
    EXPECT_EQ(result.constructed(), test_case->constructed);
    EXPECT_EQ(result.tag_number(), test_case->tag_number);
    EXPECT_EQ(result.remaining(), test_case->remaining);
  }

  harness->ReadUntil(test_harness::BYTE_TIME * 1024);
  harness->setVerbosity(verbosity);
}

TEST_F(Asn1Test, ParseTagFailureTest) {
  const int verbosity = harness->getVerbosity();
  harness->setVerbosity(verbosity - 1);
  harness->ReadUntil(test_harness::BYTE_TIME * 1024);

  for (size_t i = 0; i < ARRAYSIZE(ASN1_INVALID_TAGS); i++) {
    const asn1_tag_data *test_case = &ASN1_INVALID_TAGS[i];

    Asn1TagTest request;
    request.set_tag(test_case->tag, test_case->tag_len);

    ASSERT_NO_ERROR(harness->SendOneofProto(
        APImessageID::TESTING_API_CALL,
        OneofTestParametersCase::kAsn1TagTest,
        request));

    test_harness::raw_message msg;
    ASSERT_NO_ERROR(harness->GetData(&msg, 4096 * BYTE_TIME));
    ASSERT_MSG_TYPE(msg, APImessageID::TESTING_API_RESPONSE);
    ASSERT_SUBTYPE(msg, OneofTestResultsCase::kAsn1TagTestResult);

    Asn1TagTestResult result;
    ASSERT_TRUE(result.ParseFromArray(reinterpret_cast<char *>(msg.data + 2),
                                      msg.data_len - 2));
    EXPECT_EQ(result.result_code(), DcryptError::ASN1_TAG_PARSE_FAILURE)
      << result.result_code() << " is "
      << DcryptError_Name(result.result_code());
  }

  harness->ReadUntil(test_harness::BYTE_TIME * 1024);
  harness->setVerbosity(verbosity);
}

// TODO: add support for X509 parsing, signature validation.

}  // namespace
