
#include <app_nugget.h>
#include <nos/NuggetClientInterface.h>
#include <gtest/gtest.h>

#include <chrono>
#include <memory>

#include "nugget_tools.h"
#include "util.h"


using std::string;
using std::vector;
using std::unique_ptr;

namespace {

class NuggetCoreTest: public testing::Test {
 protected:
  static void SetUpTestCase();
  static void TearDownTestCase();

  static unique_ptr<nos::NuggetClientInterface> client;
  static vector<uint8_t> input_buffer;
  static vector<uint8_t> output_buffer;
};

unique_ptr<nos::NuggetClientInterface> NuggetCoreTest::client;

vector<uint8_t> NuggetCoreTest::input_buffer;
vector<uint8_t> NuggetCoreTest::output_buffer;

void NuggetCoreTest::SetUpTestCase() {
  client = nugget_tools::MakeNuggetClient();
  client->Open();
  input_buffer.reserve(0x4000);
  output_buffer.reserve(0x4000);
  EXPECT_TRUE(client->IsOpen()) << "Unable to connect";
}

void NuggetCoreTest::TearDownTestCase() {
  client->Close();
  client = unique_ptr<nos::NuggetClientInterface>();
}

// ./test_app --id 0 -p 0 -a
TEST_F(NuggetCoreTest, GetVersionStringTest) {
  input_buffer.resize(0);
  ASSERT_NO_ERROR(NuggetCoreTest::client->CallApp(
      APP_ID_NUGGET, NUGGET_PARAM_VERSION, input_buffer, &output_buffer));
  ASSERT_GT(output_buffer.size(), 0u);
}

TEST_F(NuggetCoreTest, SoftRebootTest) {
  ASSERT_TRUE(nugget_tools::RebootNugget(client.get(), NUGGET_REBOOT_SOFT));
}

TEST_F(NuggetCoreTest, HardRebootTest) {
  ASSERT_TRUE(nugget_tools::RebootNugget(client.get(), NUGGET_REBOOT_HARD));
}

TEST_F(NuggetCoreTest, GetLowPowerStats) {
  vector<uint8_t> buffer;
  buffer.reserve(10);                           // TBD. No results yet.
  ASSERT_NO_ERROR(NuggetCoreTest::client->CallApp(
      APP_ID_NUGGET, NUGGET_PARAM_GET_LOW_POWER_STATS,
      buffer, &buffer));
  // TODO(b/70510004): Verify meaningful values
  ASSERT_EQ(buffer.size(), 0u);
}

}  // namespace
