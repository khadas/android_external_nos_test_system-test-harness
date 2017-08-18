
#include <algorithm>
#include <memory>

#include "gtest/gtest.h"
#include "src/util.h"

namespace {

class AesTestFixture: public testing::Test {
 protected:
  virtual void SetUp() {
    harness = std::unique_ptr<test_harness::TestHarness>(
        new test_harness::TestHarness());

    EXPECT_TRUE(harness->ttyState());
  }

  virtual void TearDown() {
    harness = std::unique_ptr<test_harness::TestHarness>();
  }

 public:
  std::unique_ptr<test_harness::TestHarness> harness;
};

TEST_F(AesTestFixture, SendRaw) {
  const char content[] = "This is a test message.";

  test_harness::raw_message msg;
  msg.type = 0;
  std::copy(content, content + sizeof(content), msg.data);
  msg.data_len = sizeof(content);

  EXPECT_EQ(harness->sendAhdlc(msg), 0);
  harness->flushUntil(test_harness::BYTE_TIME * 1024);
}

}  // namespace
