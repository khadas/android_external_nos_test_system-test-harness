
#include "gtest/gtest.h"
#include "nugget_driver.h"
#include "weaver/weaver.pb.h"

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

#define ASSERT_NO_ERROR(code) \
  ASSERT_EQ(code, app_status::APP_SUCCESS) \
      << code << " is " << errorString(code)

TEST_F(WeaverTest, GetConfig) {
  uint32_t retval = -1;
  GetConfigRequest request;
  GetConfigResponse response;

  // TODO implement protoc-gen-gugget-client-cc
  /*assert(ns(Weaver_GetConfig)(&request, &response, &retval));
  assert(retval == APP_SUCCESS);
  assert(response.number_of_slots == 64);
  assert(response.key_size == 16);
  assert(response.value_size == 16);*/
}


}  // namespace
