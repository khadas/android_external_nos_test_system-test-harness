
#include "gtest/gtest.h"
#include "nugget_driver.h"
#include "weaver.pb.h"
#include "Weaver.client.h"

using nugget_driver::buf;
using nugget_driver::bufsize;
using std::cout;
using std::string;

using namespace nugget::app::weaver;

namespace {

class WeaverTest: public testing::Test {
 protected:
  static void SetUpTestCase();
  static void TearDownTestCase();
};

void WeaverTest::SetUpTestCase() {
  EXPECT_TRUE(nugget_driver::OpenDevice()) << "Unable to connect";
}

void WeaverTest::TearDownTestCase() {
  nugget_driver::CloseDevice();
}

TEST_F(WeaverTest, GetConfig) {
  GetConfigRequest request;
  GetConfigResponse response;

  Weaver service;
  ASSERT_NO_ERROR(service.GetConfig(request, response));
  EXPECT_EQ(response.number_of_slots(), 64);
  EXPECT_EQ(response.key_size(), 16);
  EXPECT_EQ(response.value_size(), 16);
}


}  // namespace
