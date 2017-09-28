#include "gtest/gtest.h"
#include "nugget_tools.h"
#include "nugget/app/keymaster/keymaster.pb.h"
#include "Keymaster.client.h"

#include "src/include/keymaster/hal/3.0/types.h"

using std::cout;
using std::string;
using std::unique_ptr;

using namespace android::hardware::keymaster::V3_0;

using namespace nugget::app::keymaster;

namespace {

class KeymasterTest: public testing::Test {
 protected:
  static unique_ptr<nos::NuggetClient> client;

  static void SetUpTestCase();
  static void TearDownTestCase();
};

unique_ptr<nos::NuggetClient> KeymasterTest::client;

void KeymasterTest::SetUpTestCase() {
  client = nugget_tools::MakeNuggetClient();
  client->Open();
  EXPECT_TRUE(client->IsOpen()) << "Unable to connect";
}

void KeymasterTest::TearDownTestCase() {
  client->Close();
  client = unique_ptr<nos::NuggetClient>();
}

// Failure cases.
TEST_F(KeymasterTest, ImportKeyAlgorithmMissingFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  KeyParameters *params = request.mutable_params();

  /* Algorithm tag is unspecified, import should fail. */
  KeyParameter *param = params->add_params();
  param->set_tag((uint32_t)Tag::KEY_SIZE);
  param->set_integer(512);

  param = params->add_params();
  param->set_tag((uint32_t)Tag::RSA_PUBLIC_EXPONENT);
  param->set_integer(3);

  Keymaster service(*client);
  ASSERT_NO_ERROR(service.ImportKey(request, &response));
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::INVALID_ARGUMENT);
}

TEST_F(KeymasterTest, ImportKeyRSAInvalidKeySizeFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  KeyParameters *params = request.mutable_params();
  KeyParameter *param = params->add_params();
  param->set_tag((uint32_t)Tag::ALGORITHM);
  param->set_integer((uint32_t)Algorithm::RSA);

  param = params->add_params();
  param->set_tag((uint32_t)Tag::KEY_SIZE);
  param->set_integer(256);

  param = params->add_params();
  param->set_tag((uint32_t)Tag::RSA_PUBLIC_EXPONENT);
  param->set_integer(3);

  Keymaster service(*client);
  ASSERT_NO_ERROR(service.ImportKey(request, &response));
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::UNSUPPORTED_KEY_SIZE);
}

TEST_F(KeymasterTest, ImportKeyRSAInvalidPublicExponentFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  KeyParameters *params = request.mutable_params();
  KeyParameter *param = params->add_params();
  param->set_tag((uint32_t)Tag::ALGORITHM);
  param->set_integer((uint32_t)Algorithm::RSA);

  param = params->add_params();
  param->set_tag((uint32_t)Tag::KEY_SIZE);
  param->set_integer(512);

  param = params->add_params();
  param->set_tag((uint32_t)Tag::RSA_PUBLIC_EXPONENT);
  param->set_integer(2);      // Unsupported exponent.

  Keymaster service(*client);
  ASSERT_NO_ERROR(service.ImportKey(request, &response));
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::INVALID_ARGUMENT);
}

TEST_F(KeymasterTest, ImportKeyRSAKeySizeTagMisatchFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  KeyParameters *params = request.mutable_params();
  KeyParameter *param = params->add_params();
  param->set_tag((uint32_t)Tag::ALGORITHM);
  param->set_integer((uint32_t)Algorithm::RSA);

  param = params->add_params();
  param->set_tag((uint32_t)Tag::KEY_SIZE);
  param->set_integer(512);

  param = params->add_params();
  param->set_tag((uint32_t)Tag::RSA_PUBLIC_EXPONENT);
  param->set_integer(3);

  request.mutable_rsa()->set_e(3);
  request.mutable_rsa()->set_d(std::string(64, '\0'));
  request.mutable_rsa()->set_n(
      std::string(63, '\0'));    // N does not match KEY_SIZE.

  Keymaster service(*client);
  ASSERT_NO_ERROR(service.ImportKey(request, &response));
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::INVALID_ARGUMENT);

  request.mutable_rsa()->set_d(
      std::string(63, '\0'));    // D does not match KEY_SIZE.
  request.mutable_rsa()->set_n(std::string(64, '\0'));

  ASSERT_NO_ERROR(service.ImportKey(request, &response));
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::INVALID_ARGUMENT);
}

TEST_F(KeymasterTest, ImportKeyRSAPublicExponentTagMisatchFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  KeyParameters *params = request.mutable_params();
  KeyParameter *param = params->add_params();
  param->set_tag((uint32_t)Tag::ALGORITHM);
  param->set_integer((uint32_t)Algorithm::RSA);

  param = params->add_params();
  param->set_tag((uint32_t)Tag::KEY_SIZE);
  param->set_integer(512);

  param = params->add_params();
  param->set_tag((uint32_t)Tag::RSA_PUBLIC_EXPONENT);
  param->set_integer(3);

  // e does not match PUBLIC_EXPONENT tag.
  request.mutable_rsa()->set_e(2);

  Keymaster service(*client);
  ASSERT_NO_ERROR(service.ImportKey(request, &response));
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::INVALID_ARGUMENT);
}

// TODO: import bad keys (n, e, d) mismatch cases.

// TODO: import key success cases

}  // namespace
