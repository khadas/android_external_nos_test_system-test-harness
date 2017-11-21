
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

// ./test_app --id 0 -p beef a b c d e f
TEST_F(NuggetCoreTest, ReverseStringTest) {
  const char test_string[] = "a b c d e f";
  ASSERT_LT(sizeof(test_string), input_buffer.capacity());
  input_buffer.resize(sizeof(test_string));
  std::copy(test_string, test_string + sizeof(test_string),
            input_buffer.begin());

  ASSERT_NO_ERROR(NuggetCoreTest::client->CallApp(
      APP_ID_NUGGET, NUGGET_PARAM_REVERSE, input_buffer, &output_buffer));

  ASSERT_EQ(output_buffer.size(), sizeof(test_string));

  for (size_t x = 0; x < sizeof(test_string); ++x) {
    ASSERT_EQ(test_string[sizeof(test_string) - 1 - x],
              output_buffer[x]) << "Failure at index: " << x;
  }
}

TEST_F(NuggetCoreTest, SoftRebootTest) {
  ASSERT_TRUE(nugget_tools::RebootNugget(client.get(), NUGGET_REBOOT_SOFT));
}

TEST_F(NuggetCoreTest, HardRebootTest) {
  ASSERT_TRUE(nugget_tools::RebootNugget(client.get(), NUGGET_REBOOT_HARD));
}


}  // namespace
