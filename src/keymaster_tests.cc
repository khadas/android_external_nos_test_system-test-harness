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
  static unique_ptr<Keymaster> service;

  static void SetUpTestCase();
  static void TearDownTestCase();

  void initRequest(ImportKeyRequest *request, Algorithm alg) {
    KeyParameters *params = request->mutable_params();
    KeyParameter *param = params->add_params();
    param->set_tag((uint32_t)Tag::ALGORITHM);
    param->set_integer((uint32_t)alg);
  }

  void initRequest(ImportKeyRequest *request, Algorithm alg, int key_size) {
    initRequest(request, alg);

    if (key_size >= 0) {
      KeyParameters *params = request->mutable_params();
      KeyParameter *param = params->add_params();
      param->set_tag((uint32_t)Tag::KEY_SIZE);
      param->set_integer(key_size);
    }
  }

  void initRequest(ImportKeyRequest *request, Algorithm alg, int key_size,
                   int public_exponent_tag) {
    initRequest(request, alg, key_size);

    if (public_exponent_tag >= 0) {
      KeyParameters *params = request->mutable_params();
      KeyParameter *param = params->add_params();
      param->set_tag((uint32_t)Tag::RSA_PUBLIC_EXPONENT);
      param->set_integer(public_exponent_tag);
    }
  }

  void initRequest(ImportKeyRequest *request, Algorithm alg, int key_size,
                   int public_exponent_tag, uint32_t public_exponent,
                   const string& d, const string& n) {
    initRequest(request, alg, key_size, public_exponent_tag);

    request->mutable_rsa()->set_e(public_exponent);
    request->mutable_rsa()->set_d(d);
    request->mutable_rsa()->set_n(n);
  }
};

unique_ptr<nos::NuggetClient> KeymasterTest::client;
unique_ptr<Keymaster> KeymasterTest::service;

void KeymasterTest::SetUpTestCase() {
  client = nugget_tools::MakeNuggetClient();
  client->Open();
  EXPECT_TRUE(client->IsOpen()) << "Unable to connect";

  service.reset(new Keymaster(*client));
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

  ASSERT_NO_ERROR(service->ImportKey(request, &response));
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::INVALID_ARGUMENT);
}

TEST_F(KeymasterTest, ImportKeyRSAInvalidKeySizeFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  initRequest(&request, Algorithm::RSA, 256, 3);

  ASSERT_NO_ERROR(service->ImportKey(request, &response));
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::UNSUPPORTED_KEY_SIZE);
}

TEST_F(KeymasterTest, ImportKeyRSAInvalidPublicExponentFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  // Unsupported exponent
  initRequest(&request, Algorithm::RSA, 512, 2, 2,
              string(64, '\0'), string(64, '\0'));

  ASSERT_NO_ERROR(service->ImportKey(request, &response));
  EXPECT_EQ((ErrorCode)response.error_code(),
            ErrorCode::UNSUPPORTED_KEY_SIZE);
}

TEST_F(KeymasterTest, ImportKeyRSAKeySizeTagMisatchNFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  // N does not match KEY_SIZE.
  initRequest(&request, Algorithm::RSA, 512, 3, 3,
              string(64, '\0'), string(63, '\0'));
  ASSERT_NO_ERROR(service->ImportKey(request, &response));
  EXPECT_EQ((ErrorCode)response.error_code(),
            ErrorCode::IMPORT_PARAMETER_MISMATCH);
}

TEST_F(KeymasterTest, ImportKeyRSAKeySizeTagMisatchDFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  // D does not match KEY_SIZE.
  initRequest(&request, Algorithm::RSA, 512, 3, 3,
              string(63, '\0'), string(64, '\0'));
  ASSERT_NO_ERROR(service->ImportKey(request, &response));
  EXPECT_EQ((ErrorCode)response.error_code(),
            ErrorCode::IMPORT_PARAMETER_MISMATCH);
}

TEST_F(KeymasterTest, ImportKeyRSAPublicExponentTagMisatchFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  // e does not match PUBLIC_EXPONENT tag.
  initRequest(&request, Algorithm::RSA, 512, 3, 2,
              string(64, '\0'), string(64, '\0'));
  ASSERT_NO_ERROR(service->ImportKey(request, &response));
  EXPECT_EQ((ErrorCode)response.error_code(),
            ErrorCode::IMPORT_PARAMETER_MISMATCH);
}

TEST_F(KeymasterTest, ImportKeyRSA1024BadEFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  // Mis-matched e.
  const string d((const char *)RSA_1024_D, sizeof(RSA_1024_D));
  const string N((const char *)RSA_1024_N, sizeof(RSA_1024_N));
  initRequest(&request, Algorithm::RSA, 1024, 3, 3, d, N);

  ASSERT_NO_ERROR(service->ImportKey(request, &response));
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::INVALID_ARGUMENT);
}

TEST_F(KeymasterTest, ImportKeyRSA1024BadDFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  const string d(string("\x01") +  /* Twiddle LSB of D. */
                 string((const char *)RSA_1024_D, sizeof(RSA_1024_D) - 1));
  const string N((const char *)RSA_1024_N, sizeof(RSA_1024_N));
  initRequest(&request, Algorithm::RSA, 1024, 65537, 65537, d, N);

  ASSERT_NO_ERROR(service->ImportKey(request, &response));
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::INVALID_ARGUMENT);
}

TEST_F(KeymasterTest, ImportKeyRSA1024BadNFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  const string d((const char *)RSA_1024_D, sizeof(RSA_1024_D));
  const string N(string("\x01") +  /* Twiddle LSB of N. */
                 string((const char *)RSA_1024_N, sizeof(RSA_1024_N) - 1));
  initRequest(&request, Algorithm::RSA, 1024, 65537, 65537, d, N);

  ASSERT_NO_ERROR(service->ImportKey(request, &response));
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::INVALID_ARGUMENT);
}

TEST_F(KeymasterTest, ImportKeyRSASuccess) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  initRequest(&request, Algorithm::RSA);
  KeyParameters *params = request.mutable_params();
  KeyParameter *param = params->add_params();
  for (size_t i = 0; i < ARRAYSIZE(TEST_RSA_KEYS); i++) {
    param->set_tag((uint32_t)Tag::RSA_PUBLIC_EXPONENT);
    param->set_integer(TEST_RSA_KEYS[i].e);

    request.mutable_rsa()->set_e(TEST_RSA_KEYS[i].e);
    request.mutable_rsa()->set_d(TEST_RSA_KEYS[i].d, TEST_RSA_KEYS[i].size);
    request.mutable_rsa()->set_n(TEST_RSA_KEYS[i].n, TEST_RSA_KEYS[i].size);

    ASSERT_NO_ERROR(service->ImportKey(request, &response))
        << "Failed at TEST_RSA_KEYS[" << i << "]";
    EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::OK)
        << "Failed at TEST_RSA_KEYS[" << i << "]";
  }
}

TEST_F(KeymasterTest, ImportKeyRSA1024OptionalParamsAbsentSuccess) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  initRequest(&request, Algorithm::RSA, -1, -1, 65537,
              string((const char *)RSA_1024_D, sizeof(RSA_1024_D)),
              string((const char *)RSA_1024_N, sizeof(RSA_1024_N)));

  ASSERT_NO_ERROR(service->ImportKey(request, &response));
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::OK);
}

// TODO: import bad keys (n, e, d) mismatch cases.

// TODO: import key success cases

}  // namespace
