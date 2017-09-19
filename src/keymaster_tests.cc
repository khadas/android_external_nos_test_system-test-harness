#include "gtest/gtest.h"
#include "nugget_tools.h"
#include "keymaster.pb.h"
#include "Keymaster.client.h"

using std::cout;
using std::string;
using std::unique_ptr;

using namespace nugget::app::keymaster;

namespace {

class KeymasterTest: public testing::Test {
 protected:
  static unique_ptr<nos::LinuxCitadelClient> citadelClient;

  static void SetUpTestCase();
  static void TearDownTestCase();

 public:
  const size_t KEY_SIZE = 16;
};

unique_ptr<nos::LinuxCitadelClient> KeymasterTest::citadelClient;

void KeymasterTest::SetUpTestCase() {
  citadelClient =
      unique_ptr<nos::LinuxCitadelClient>(new nos::LinuxCitadelClient(
          nugget_tools::getNosCoreFreq(), nugget_tools::getNosCoreSerial()));
  citadelClient->open();
  EXPECT_TRUE(citadelClient->isOpen()) << "Unable to connect";
}

void KeymasterTest::TearDownTestCase() {
  citadelClient->close();
  citadelClient = unique_ptr<nos::LinuxCitadelClient>();
}

TEST_F(KeymasterTest, GetHardwareFeatures) {
  GetHardwareFeaturesRequest request;
  GetHardwareFeaturesResponse response;

  Keymaster service(*citadelClient);
  ASSERT_NO_ERROR(service.GetHardwareFeatures(request, response));
  EXPECT_EQ(response.is_secure(), true);
  EXPECT_EQ(response.supports_elliptic_curve(), false);
  EXPECT_EQ(response.supports_symmetric_cryptography(), false);
  EXPECT_EQ(response.supports_attestation(), false);
  EXPECT_EQ(response.supports_all_digests(), false);
  EXPECT_EQ(response.keymaster_name(), "CitadelKeymaster");
  EXPECT_EQ(response.keymaster_author_name(), "Google");
}

}  // namespace
