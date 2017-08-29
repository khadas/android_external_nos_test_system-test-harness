
#include <chrono>
#include <memory>

#include "gflags/gflags.h"
#include "gtest/gtest.h"
#include "re2/re2.h"
#include "util.h"

extern "C" {
#include "core/citadel/config_chip.h"
#include "include/application.h"
#include "include/app_nugget.h"
#include "util/poker/driver.h"
}

/* These are required as globals because of transport.c */
device_t* dev;
int verbose = 0;

//DEFINE_uint32(nos_core_id, 0x00, "App ID (default 0x00)");
//DEFINE_uint32(nos_core_param, 0x00, "Set the command Param field to HEX");
DEFINE_int32(nos_core_freq, 10000000, "SPI clock frequency (default 10MHz)");
DEFINE_string(nos_core_serial, "", "USB device serial number to open");

using std::cout;
using std::string;
using std::vector;
using std::unique_ptr;

namespace {

class NuggetCoreTest: public testing::Test {
 protected:
  static void SetUpTestCase();
  static void TearDownTestCase();

  static const char* errorString(uint32_t code);

 public:
  static const size_t bufsize = 0x4000;
  static uint8_t buf[bufsize];
};

const size_t NuggetCoreTest::bufsize;
uint8_t NuggetCoreTest::buf[NuggetCoreTest::bufsize];

void NuggetCoreTest::SetUpTestCase() {
  dev = OpenDev(FLAGS_nos_core_freq,
                FLAGS_nos_core_serial.size() ? FLAGS_nos_core_serial.c_str() :
                NULL);
  EXPECT_NE(dev, (void *) NULL) << "Unable to connect";
}

void NuggetCoreTest::TearDownTestCase() {
  CloseDev(dev);
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
  ASSERT_LT(sizeof(test_string), NuggetCoreTest::bufsize);
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
  result = harness.readUntil(std::chrono::microseconds(2025000));
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
  result = harness.readUntil(std::chrono::microseconds(2025000));
  ASSERT_TRUE(RE2::PartialMatch(result, "\\[Reset cause: ap-off\\]"));
}


}  // namespace
