
#include <chrono>

#include "gflags/gflags.h"
#include "gtest/gtest.h"
#include "nugget_driver.h"
#include "re2/re2.h"
#include "util.h"

using std::cout;
using std::string;
using std::vector;

using nugget_driver::buf;

namespace {

class NuggetCoreTest: public testing::Test {
 protected:
  static void SetUpTestCase();
  static void TearDownTestCase();

  static const char* errorString(uint32_t code);
};

void NuggetCoreTest::SetUpTestCase() {
  EXPECT_TRUE(nugget_driver::OpenDevice()) << "Unable to connect";
}

void NuggetCoreTest::TearDownTestCase() {
  nugget_driver::CloseDevice();
}

const char* NuggetCoreTest::errorString(uint32_t code) {
  switch (code) {
    case app_status::APP_SUCCESS:
      return "APP_SUCCESS";
    case app_status::APP_ERROR_BOGUS_ARGS:
      return "APP_ERROR_BOGUS_ARGS";
    case app_status::APP_ERROR_INTERNAL:
      return "APP_ERROR_INTERNAL";
    case app_status::APP_ERROR_TOO_MUCH:
      return "APP_ERROR_TOO_MUCH";
    default:
      return "unknown";
  }
}

#define ASSERT_NO_ERROR(code) \
  ASSERT_EQ(code, app_status::APP_SUCCESS) \
      << code << " is " << errorString(code)

// ./test_app --id 0 -p 0 -a
TEST_F(NuggetCoreTest, GetVersionStringTest) {
  uint32_t replycount;
  ASSERT_NO_ERROR(call_application(0x00, 0x00, (uint8_t*) buf, 0,
                                   (uint8_t*) buf, &replycount));
  ASSERT_GT(replycount, 0);
  cout << string((char*) buf, replycount) <<"\n";
  cout.flush();
}

// ./test_app --id 0 -p beef a b c d e f
TEST_F(NuggetCoreTest, ReverseStringTest) {
  const char test_string[] = "a b c d e f";
  ASSERT_LT(sizeof(test_string), nugget_driver::bufsize);
  std::copy(test_string, test_string + sizeof(test_string), buf);

  uint32_t replycount;
  ASSERT_NO_ERROR(call_application(0x00, 0xbeef, (uint8_t*) buf,
                                   sizeof(test_string), (uint8_t*) buf,
                                   &replycount));

  ASSERT_EQ(replycount, sizeof(test_string));

  for (size_t x = 0; x < sizeof(test_string); ++x) {
    ASSERT_EQ(test_string[sizeof(test_string) - 1 - x],
              buf[x]) << "Failure at index: " << x;
  }
}

// matches [97.721660 Rebooting in 2 seconds]\x0d
RE2 reboot_message_matcher("\\[[0-9]+\\.[0-9]+ Rebooting in [0-9]+ seconds\\]");
const auto REBOOT_DELAY = std::chrono::microseconds(2250000);

TEST_F(NuggetCoreTest, SoftRebootTest) {
  test_harness::TestHarness harness;

  buf[0] = 0;  // 0 = soft reboot, 1 = hard reboot
  uint32_t replycount;
  ASSERT_NO_ERROR(call_application(0x00, 0x0002, (uint8_t*) buf, 1,
                                   (uint8_t*) buf, &replycount));
  ASSERT_EQ(replycount, 0);

  string result = harness.readUntil(1042 * test_harness::BYTE_TIME);
  ASSERT_TRUE(RE2::PartialMatch(result, reboot_message_matcher));
  NuggetCoreTest::TearDownTestCase();
  NuggetCoreTest::SetUpTestCase();
  result = harness.readUntil(REBOOT_DELAY);
  ASSERT_TRUE(RE2::PartialMatch(
      result, "\\[Reset cause: hibernate rtc-alarm\\]"));
}

TEST_F(NuggetCoreTest, HardRebootTest) {
  test_harness::TestHarness harness;

  buf[0] = 1;  // 0 = soft reboot, 1 = hard reboot
  uint32_t replycount;
  ASSERT_NO_ERROR(call_application(0x00, 0x0002, (uint8_t*) buf, 1,
                                   (uint8_t*) buf, &replycount));
  ASSERT_EQ(replycount, 0);

  string result = harness.readUntil(1042 * test_harness::BYTE_TIME);
  ASSERT_TRUE(RE2::PartialMatch(result, reboot_message_matcher));
  NuggetCoreTest::TearDownTestCase();
  NuggetCoreTest::SetUpTestCase();
  result = harness.readUntil(REBOOT_DELAY);
  ASSERT_TRUE(RE2::PartialMatch(result, "\\[Reset cause: ap-off\\]"));
}


}  // namespace
