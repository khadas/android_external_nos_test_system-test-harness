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
  static unique_ptr<nos::linux::CitadelClient> citadelClient;

  static void SetUpTestCase();
  static void TearDownTestCase();
};

unique_ptr<nos::linux::CitadelClient> KeymasterTest::citadelClient;

void KeymasterTest::SetUpTestCase() {
  citadelClient =
      unique_ptr<nos::linux::CitadelClient>(new nos::linux::CitadelClient(
          nugget_tools::getNosCoreFreq(), nugget_tools::getNosCoreSerial()));
  citadelClient->open();
  EXPECT_TRUE(citadelClient->isOpen()) << "Unable to connect";
}

void KeymasterTest::TearDownTestCase() {
  citadelClient->close();
  citadelClient = unique_ptr<nos::linux::CitadelClient>();
}

}  // namespace
