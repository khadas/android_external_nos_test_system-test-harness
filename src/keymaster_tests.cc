#include "gtest/gtest.h"
#include "nugget_tools.h"
#include "nugget/app/keymaster/keymaster.pb.h"
#include "Keymaster.client.h"

#include "src/macros.h"
#include "src/include/keymaster/hal/3.0/types.h"
#include "src/test-data/test-keys/rsa.h"

using std::cout;
using std::string;
using std::unique_ptr;

using namespace android::hardware::keymaster::V3_0;

using namespace nugget::app::keymaster;

using namespace test_data;

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
  EXPECT_EQ((ErrorCode)response.error_code(),
            ErrorCode::IMPORT_PARAMETER_MISMATCH);
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
  request.mutable_rsa()->set_d(string(64, '\0'));
  request.mutable_rsa()->set_n(
      string(63, '\0'));    // N does not match KEY_SIZE.

  Keymaster service(*client);
  ASSERT_NO_ERROR(service.ImportKey(request, &response));
  EXPECT_EQ((ErrorCode)response.error_code(),
            ErrorCode::IMPORT_PARAMETER_MISMATCH);

  request.mutable_rsa()->set_d(
      string(63, '\0'));    // D does not match KEY_SIZE.
  request.mutable_rsa()->set_n(string(64, '\0'));

  ASSERT_NO_ERROR(service.ImportKey(request, &response));
  EXPECT_EQ((ErrorCode)response.error_code(),
            ErrorCode::IMPORT_PARAMETER_MISMATCH);
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
  EXPECT_EQ((ErrorCode)response.error_code(),
            ErrorCode::IMPORT_PARAMETER_MISMATCH);
}

TEST_F(KeymasterTest, ImportKeyRSA1024BadKeyFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  KeyParameters *params = request.mutable_params();
  KeyParameter *param = params->add_params();
  param->set_tag((uint32_t)Tag::ALGORITHM);
  param->set_integer((uint32_t)Algorithm::RSA);

  param = params->add_params();
  param->set_tag((uint32_t)Tag::KEY_SIZE);
  param->set_integer(1024);

  request.mutable_rsa()->set_e(65537);
  request.mutable_rsa()->set_d(
      string("\x01") +  /* Twiddle LSB of D. */
      string((const char *)RSA_1024_D, sizeof(RSA_1024_D) - 1));
  request.mutable_rsa()->set_n(RSA_1024_N, sizeof(RSA_1024_N));

  Keymaster service(*client);
  ASSERT_NO_ERROR(service.ImportKey(request, &response));
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::INVALID_ARGUMENT);

  request.mutable_rsa()->set_e(65537);
  request.mutable_rsa()->set_d(RSA_1024_D, sizeof(RSA_1024_D));
  request.mutable_rsa()->set_n(
      string("\x01") +  /* Twiddle LSB of N. */
      string((const char *)RSA_1024_N, sizeof(RSA_1024_N) - 1));

  ASSERT_NO_ERROR(service.ImportKey(request, &response));
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::INVALID_ARGUMENT);

  request.mutable_rsa()->set_e(3);  /* Mis-matched e. */
  request.mutable_rsa()->set_d(RSA_1024_D, sizeof(RSA_1024_D));
  request.mutable_rsa()->set_d(RSA_1024_N, sizeof(RSA_1024_N));

  ASSERT_NO_ERROR(service.ImportKey(request, &response));
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::INVALID_ARGUMENT);
}

TEST_F(KeymasterTest, ImportKeyRSASuccess) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  KeyParameters *params = request.mutable_params();
  KeyParameter *param = params->add_params();
  param->set_tag((uint32_t)Tag::ALGORITHM);
  param->set_integer((uint32_t)Algorithm::RSA);

  param = params->add_params();

  Keymaster service(*client);
  for (size_t i = 0; i < ARRAYSIZE(TEST_RSA_KEYS); i++) {
    param->set_tag((uint32_t)Tag::RSA_PUBLIC_EXPONENT);
    param->set_integer(TEST_RSA_KEYS[i].e);

    request.mutable_rsa()->set_e(TEST_RSA_KEYS[i].e);
    request.mutable_rsa()->set_d(TEST_RSA_KEYS[i].d, TEST_RSA_KEYS[i].size);
    request.mutable_rsa()->set_n(TEST_RSA_KEYS[i].n, TEST_RSA_KEYS[i].size);

    ASSERT_NO_ERROR(service.ImportKey(request, &response));
    EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::OK);
  }
}

TEST_F(KeymasterTest, ImportKeyRSA1024OptionalParamsAbsentSuccess) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  KeyParameters *params = request.mutable_params();
  KeyParameter *param = params->add_params();
  param->set_tag((uint32_t)Tag::ALGORITHM);
  param->set_integer((uint32_t)Algorithm::RSA);

  request.mutable_rsa()->set_e(65537);
  request.mutable_rsa()->set_d(RSA_1024_D, sizeof(RSA_1024_D));
  request.mutable_rsa()->set_n(RSA_1024_N, sizeof(RSA_1024_N));

  Keymaster service(*client);
  ASSERT_NO_ERROR(service.ImportKey(request, &response));
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::OK);
}

// TODO: import bad keys (n, e, d) mismatch cases.

// TODO: import key success cases

}  // namespace
