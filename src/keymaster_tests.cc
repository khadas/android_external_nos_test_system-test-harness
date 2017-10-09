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
  static unique_ptr<nos::NuggetClient> client;

  static void SetUpTestCase();
  static void TearDownTestCase();
};

unique_ptr<nos::NuggetClient> KeymasterTest::client;

void KeymasterTest::SetUpTestCase() {
  client =
      unique_ptr<nos::NuggetClient>(new nos::NuggetClient(
          nugget_tools::getNosCoreSerial()));
  client->Open();
  EXPECT_TRUE(client->IsOpen()) << "Unable to connect";
}

void KeymasterTest::TearDownTestCase() {
  client->Close();
  client = unique_ptr<nos::NuggetClient>();
}

}  // namespace
