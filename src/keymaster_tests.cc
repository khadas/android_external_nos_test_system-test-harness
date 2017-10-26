#include "gtest/gtest.h"
#include "nugget_tools.h"
#include "nugget/app/keymaster/keymaster.pb.h"
#include "Keymaster.client.h"

#include "src/macros.h"
#include "src/include/keymaster/hal/3.0/types.h"
#include "src/test-data/test-keys/rsa.h"

#include "openssl/bn.h"
#include "openssl/ec_key.h"
#include "openssl/nid.h"

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

  void initRSARequest(ImportKeyRequest *request, Algorithm alg) {
    KeyParameters *params = request->mutable_params();
    KeyParameter *param = params->add_params();
    param->set_tag((uint32_t)Tag::ALGORITHM);
    param->set_integer((uint32_t)alg);
  }

  void initRSARequest(ImportKeyRequest *request, Algorithm alg, int key_size) {
    initRSARequest(request, alg);

    if (key_size >= 0) {
      KeyParameters *params = request->mutable_params();
      KeyParameter *param = params->add_params();
      param->set_tag((uint32_t)Tag::KEY_SIZE);
      param->set_integer(key_size);
    }
  }

  void initRSARequest(ImportKeyRequest *request, Algorithm alg, int key_size,
                   int public_exponent_tag) {
    initRSARequest(request, alg, key_size);

    if (public_exponent_tag >= 0) {
      KeyParameters *params = request->mutable_params();
      KeyParameter *param = params->add_params();
      param->set_tag((uint32_t)Tag::RSA_PUBLIC_EXPONENT);
      param->set_long_integer(public_exponent_tag);
    }
  }

  void initRSARequest(ImportKeyRequest *request, Algorithm alg, int key_size,
                   int public_exponent_tag, uint32_t public_exponent,
                   const string& d, const string& n) {
    initRSARequest(request, alg, key_size, public_exponent_tag);

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

// TODO: refactor into import key tests.

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
  param->set_long_integer(3);

  ASSERT_NO_ERROR(service->ImportKey(request, &response));
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::INVALID_ARGUMENT);
}

// RSA

TEST_F(KeymasterTest, ImportKeyRSAInvalidKeySizeFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  initRSARequest(&request, Algorithm::RSA, 256, 3);

  ASSERT_NO_ERROR(service->ImportKey(request, &response));
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::UNSUPPORTED_KEY_SIZE);
}

TEST_F(KeymasterTest, ImportKeyRSAInvalidPublicExponentFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  // Unsupported exponent
  initRSARequest(&request, Algorithm::RSA, 512, 2, 2,
                 string(64, '\0'), string(64, '\0'));

  ASSERT_NO_ERROR(service->ImportKey(request, &response));
  EXPECT_EQ((ErrorCode)response.error_code(),
            ErrorCode::UNSUPPORTED_KEY_SIZE);
}

TEST_F(KeymasterTest, ImportKeyRSAKeySizeTagMisatchNFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  // N does not match KEY_SIZE.
  initRSARequest(&request, Algorithm::RSA, 512, 3, 3,
                 string(64, '\0'), string(63, '\0'));
  ASSERT_NO_ERROR(service->ImportKey(request, &response));
  EXPECT_EQ((ErrorCode)response.error_code(),
            ErrorCode::IMPORT_PARAMETER_MISMATCH);
}

TEST_F(KeymasterTest, ImportKeyRSAKeySizeTagMisatchDFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  // D does not match KEY_SIZE.
  initRSARequest(&request, Algorithm::RSA, 512, 3, 3,
                 string(63, '\0'), string(64, '\0'));
  ASSERT_NO_ERROR(service->ImportKey(request, &response));
  EXPECT_EQ((ErrorCode)response.error_code(),
            ErrorCode::IMPORT_PARAMETER_MISMATCH);
}

TEST_F(KeymasterTest, ImportKeyRSAPublicExponentTagMisatchFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  // e does not match PUBLIC_EXPONENT tag.
  initRSARequest(&request, Algorithm::RSA, 512, 3, 2,
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
  initRSARequest(&request, Algorithm::RSA, 1024, 3, 3, d, N);

  ASSERT_NO_ERROR(service->ImportKey(request, &response));
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::INVALID_ARGUMENT);
}

TEST_F(KeymasterTest, ImportKeyRSA1024BadDFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  const string d(string("\x01") +  /* Twiddle LSB of D. */
                 string((const char *)RSA_1024_D, sizeof(RSA_1024_D) - 1));
  const string N((const char *)RSA_1024_N, sizeof(RSA_1024_N));
  initRSARequest(&request, Algorithm::RSA, 1024, 65537, 65537, d, N);

  ASSERT_NO_ERROR(service->ImportKey(request, &response));
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::INVALID_ARGUMENT);
}

TEST_F(KeymasterTest, ImportKeyRSA1024BadNFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  const string d((const char *)RSA_1024_D, sizeof(RSA_1024_D));
  const string N(string("\x01") +  /* Twiddle LSB of N. */
                 string((const char *)RSA_1024_N, sizeof(RSA_1024_N) - 1));
  initRSARequest(&request, Algorithm::RSA, 1024, 65537, 65537, d, N);

  ASSERT_NO_ERROR(service->ImportKey(request, &response));
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::INVALID_ARGUMENT);
}

TEST_F(KeymasterTest, ImportKeyRSASuccess) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  initRSARequest(&request, Algorithm::RSA);
  KeyParameters *params = request.mutable_params();
  KeyParameter *param = params->add_params();
  for (size_t i = 0; i < ARRAYSIZE(TEST_RSA_KEYS); i++) {
    param->set_tag((uint32_t)Tag::RSA_PUBLIC_EXPONENT);
    param->set_long_integer(TEST_RSA_KEYS[i].e);

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

  initRSARequest(&request, Algorithm::RSA, -1, -1, 65537,
                 string((const char *)RSA_1024_D, sizeof(RSA_1024_D)),
                 string((const char *)RSA_1024_N, sizeof(RSA_1024_N)));

  ASSERT_NO_ERROR(service->ImportKey(request, &response));
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::OK);
}

// EC

TEST_F(KeymasterTest, ImportKeyECMissingCurveIdTagFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  KeyParameters *params = request.mutable_params();
  KeyParameter *param = params->add_params();
  param->set_tag((uint32_t)Tag::ALGORITHM);
  param->set_integer((uint32_t)Algorithm::EC);

  ASSERT_NO_ERROR(service->ImportKey(request, &response));
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::INVALID_ARGUMENT);
}

TEST_F(KeymasterTest, ImportKeyECMisMatchedCurveIdTagFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  KeyParameters *params = request.mutable_params();
  KeyParameter *param = params->add_params();
  param->set_tag((uint32_t)Tag::ALGORITHM);
  param->set_integer((uint32_t)Algorithm::EC);

  param = params->add_params();
  param->set_tag((uint32_t)Tag::EC_CURVE);
  param->set_integer((uint32_t)EcCurve::P_224);

  request.mutable_ec()->set_curve_id((uint32_t)EcCurve::P_256);

  ASSERT_NO_ERROR(service->ImportKey(request, &response));
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::INVALID_ARGUMENT);
}

// TODO: tests for P224.

TEST_F(KeymasterTest, ImportKeyECMisMatchedP256KeySizeFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  KeyParameters *params = request.mutable_params();
  KeyParameter *param = params->add_params();
  param->set_tag((uint32_t)Tag::ALGORITHM);
  param->set_integer((uint32_t)Algorithm::EC);

  param = params->add_params();
  param->set_tag((uint32_t)Tag::EC_CURVE);
  param->set_integer((uint32_t)EcCurve::P_256);

  request.mutable_ec()->set_curve_id((uint32_t)EcCurve::P_256);
  request.mutable_ec()->set_d(string((224 >> 3) - 1, '\0'));
  request.mutable_ec()->set_x(string((224 >> 3), '\0'));
  request.mutable_ec()->set_y(string((224 >> 3), '\0'));

  ASSERT_NO_ERROR(service->ImportKey(request, &response));
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::INVALID_ARGUMENT);
}

// TODO: bad key tests.  invalid d, {x,y} not on curve, d, xy mismatched.
TEST_F(KeymasterTest, ImportKeyECP256BadKeyFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  KeyParameters *params = request.mutable_params();
  KeyParameter *param = params->add_params();
  param->set_tag((uint32_t)Tag::ALGORITHM);
  param->set_integer((uint32_t)Algorithm::EC);

  param = params->add_params();
  param->set_tag((uint32_t)Tag::EC_CURVE);
  param->set_integer((uint32_t)EcCurve::P_256);

  request.mutable_ec()->set_curve_id((uint32_t)EcCurve::P_256);
  request.mutable_ec()->set_d(string((224 >> 3), '\0'));
  request.mutable_ec()->set_x(string((224 >> 3), '\0'));
  request.mutable_ec()->set_y(string((224 >> 3), '\0'));

  ASSERT_NO_ERROR(service->ImportKey(request, &response));
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::INVALID_ARGUMENT);
}

TEST_F (KeymasterTest, DISABLED_ImportECP256KeySuccess) {
  // Generate an EC key.
  // TODO: just hardcode a test key.
  bssl::UniquePtr<EC_KEY> ec(EC_KEY_new_by_curve_name(NID_X9_62_prime256v1));
  EXPECT_EQ(EC_KEY_generate_key(ec.get()), 1);
  const EC_GROUP *group = EC_KEY_get0_group(ec.get());
  const BIGNUM *d = EC_KEY_get0_private_key(ec.get());
  const EC_POINT *point = EC_KEY_get0_public_key(ec.get());
  bssl::UniquePtr<BIGNUM> x(BN_new());
  bssl::UniquePtr<BIGNUM> y(BN_new());
  EXPECT_EQ(EC_POINT_get_affine_coordinates_GFp(
      group, point, x.get(), y.get(), NULL), 1);

  // Turn d, x, y into binary strings.
  std::unique_ptr<uint8_t []> dstr(new uint8_t[BN_num_bytes(d)]);
  std::unique_ptr<uint8_t []> xstr(new uint8_t[BN_num_bytes(x.get())]);
  std::unique_ptr<uint8_t []> ystr(new uint8_t[BN_num_bytes(y.get())]);

  EXPECT_EQ(BN_bn2le_padded(dstr.get(), BN_num_bytes(d), d), 1);
  EXPECT_EQ(BN_bn2le_padded(xstr.get(), BN_num_bytes(x.get()), x.get()), 1);
  EXPECT_EQ(BN_bn2le_padded(ystr.get(), BN_num_bytes(y.get()), y.get()), 1);

  ImportKeyRequest request;
  ImportKeyResponse response;

  KeyParameters *params = request.mutable_params();
  KeyParameter *param = params->add_params();
  param->set_tag((uint32_t)Tag::ALGORITHM);
  param->set_integer((uint32_t)Algorithm::EC);

  param = params->add_params();
  param->set_tag((uint32_t)Tag::EC_CURVE);
  param->set_integer((uint32_t)EcCurve::P_256);

  request.mutable_ec()->set_curve_id((uint32_t)EcCurve::P_256);
  request.mutable_ec()->set_d(dstr.get(), BN_num_bytes(d));
  request.mutable_ec()->set_x(xstr.get(), BN_num_bytes(x.get()));
  request.mutable_ec()->set_y(ystr.get(), BN_num_bytes(y.get()));

  ASSERT_NO_ERROR(service->ImportKey(request, &response));
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::OK);
}

// TODO: tests for P384, P521.

}  // namespace
