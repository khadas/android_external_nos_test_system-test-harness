
#include <chrono>
#include <memory>
#include <nos/linux/CitadelClient.h>

#include "gflags/gflags.h"
#include "gtest/gtest.h"
#include "nugget_tools.h"
#include "re2/re2.h"
#include "util.h"

extern "C" {
  // TODO: remove this need for this include
#include "core/citadel/config_chip.h"
}
#include <app_nugget.h>

using std::cout;
using std::string;
using std::vector;
using std::unique_ptr;

namespace {

class NuggetCoreTest: public testing::Test {
 protected:
  static void SetUpTestCase();
  static void TearDownTestCase();

  static unique_ptr<nos::linux::CitadelClient> citadelClient;
  static vector<uint8_t> input_buffer;
  static vector<uint8_t> output_buffer;
};

unique_ptr<nos::linux::CitadelClient> NuggetCoreTest::citadelClient;

vector<uint8_t> NuggetCoreTest::input_buffer;
vector<uint8_t> NuggetCoreTest::output_buffer;

void NuggetCoreTest::SetUpTestCase() {
  citadelClient =
      unique_ptr<nos::linux::CitadelClient>(new nos::linux::CitadelClient(
          nugget_tools::getNosCoreFreq(), nugget_tools::getNosCoreSerial()));
  citadelClient->Open();
  input_buffer.reserve(0x4000);
  output_buffer.reserve(0x4000);
  EXPECT_TRUE(citadelClient->IsOpen()) << "Unable to connect";
}

void NuggetCoreTest::TearDownTestCase() {
  citadelClient->Close();
  citadelClient = unique_ptr<nos::linux::CitadelClient>();
}

// ./test_app --id 0 -p 0 -a
TEST_F(NuggetCoreTest, GetVersionStringTest) {
  input_buffer.resize(0);
  ASSERT_NO_ERROR(NuggetCoreTest::citadelClient->CallApp(
      APP_ID_NUGGET, NUGGET_PARAM_VERSION, input_buffer, &output_buffer));
  ASSERT_GT(output_buffer.size(), 0);
  cout << string((char*) output_buffer.data(), output_buffer.size()) <<"\n";
  cout.flush();
}

// ./test_app --id 0 -p beef a b c d e f
TEST_F(NuggetCoreTest, ReverseStringTest) {
  const char test_string[] = "a b c d e f";
  ASSERT_LT(sizeof(test_string), input_buffer.capacity());
  input_buffer.resize(sizeof(test_string));
  std::copy(test_string, test_string + sizeof(test_string),
            input_buffer.begin());

  ASSERT_NO_ERROR(NuggetCoreTest::citadelClient->CallApp(
      APP_ID_NUGGET, NUGGET_PARAM_REVERSE, input_buffer, &output_buffer));

  ASSERT_EQ(output_buffer.size(), sizeof(test_string));

  for (size_t x = 0; x < sizeof(test_string); ++x) {
    ASSERT_EQ(test_string[sizeof(test_string) - 1 - x],
              output_buffer[x]) << "Failure at index: " << x;
  }
}

// matches [97.721660 Rebooting in 2 seconds]\x0d
RE2 reboot_message_matcher("\\[[0-9]+\\.[0-9]+ Rebooting in [0-9]+ seconds\\]");
const auto REBOOT_DELAY = std::chrono::microseconds(2250000);

TEST_F(NuggetCoreTest, SoftRebootTest) {
  test_harness::TestHarness harness;

  input_buffer.resize(1);
  input_buffer[0] = 0;  // 0 = soft reboot, 1 = hard reboot
  ASSERT_NO_ERROR(NuggetCoreTest::citadelClient->CallApp(
      APP_ID_NUGGET, NUGGET_PARAM_REBOOT, input_buffer, &output_buffer));
  ASSERT_EQ(output_buffer.size(), 0);

  string result = harness.ReadUntil(1042 * test_harness::BYTE_TIME);
  ASSERT_TRUE(RE2::PartialMatch(result, reboot_message_matcher));
  NuggetCoreTest::citadelClient->Close();
  result = harness.ReadUntil(REBOOT_DELAY);
  NuggetCoreTest::citadelClient->Open();
  ASSERT_TRUE(NuggetCoreTest::citadelClient->IsOpen());
  ASSERT_TRUE(RE2::PartialMatch(
      result, "\\[Reset cause: hibernate rtc-alarm\\]"));
}

// TODO(b/65930573) enable this test after it no longer breaks libnos.
TEST_F(NuggetCoreTest, DISABLED_HardRebootTest) {
  test_harness::TestHarness harness;

  input_buffer.resize(1);
  input_buffer[0] = 1;  // 0 = soft reboot, 1 = hard reboot
  ASSERT_NO_ERROR(NuggetCoreTest::citadelClient->CallApp(
      APP_ID_NUGGET, NUGGET_PARAM_REBOOT, input_buffer, &output_buffer));
  ASSERT_EQ(output_buffer.size(), 0);

  string result = harness.ReadUntil(1042 * test_harness::BYTE_TIME);
  ASSERT_TRUE(RE2::PartialMatch(result, reboot_message_matcher));
  NuggetCoreTest::citadelClient->Close();
  result = harness.ReadUntil(REBOOT_DELAY);
  NuggetCoreTest::citadelClient->Open();
  ASSERT_TRUE(NuggetCoreTest::citadelClient->IsOpen());
  ASSERT_TRUE(RE2::PartialMatch(result, "\\[Reset cause: ap-off\\]"));
}


}  // namespace
