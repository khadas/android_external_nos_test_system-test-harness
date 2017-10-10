
#include <memory>
#include <random>

#include "gtest/gtest.h"
#include "nugget_tools.h"
#include "weaver.pb.h"
#include "Weaver.client.h"

using std::cout;
using std::string;
using std::unique_ptr;

using namespace nugget::app::weaver;

namespace {

class WeaverTest: public testing::Test {
 protected:
  static std::random_device random_number_generator;
  static uint32_t slot;

  static unique_ptr<nos::NuggetClient> client;

  static void SetUpTestCase();
  static void TearDownTestCase();

  void testWrite(uint32_t slot, const uint8_t *key, const uint8_t *value);
  void testRead(const uint32_t slot, const uint8_t *key, const uint8_t *value);
  void testEraseValue(uint32_t slot);
  void testReadWrongKey(uint32_t slot, const uint8_t *key,
                        uint32_t throttle_sec);
  void testReadThrottle(uint32_t slot, const uint8_t *key,
                        uint32_t throttle_sec);
 public:
  const size_t KEY_SIZE = 16;
  const size_t VALUE_SIZE = 16;
  const uint8_t TEST_KEY[16] = {1, 2, 3, 4, 5, 6, 7, 8,
                                9, 10, 11, 12, 13, 14, 15, 16};
  const uint8_t TEST_VALUE[16] = {1, 0, 3, 0, 5, 0, 7, 0,
                                  0, 10, 0, 12, 0, 14, 0, 16};
};

std::random_device WeaverTest::random_number_generator;
/** A random slot is used for each test run to even the wear on the flash. */
uint32_t WeaverTest::slot = WeaverTest::random_number_generator() & 0x3f;

unique_ptr<nos::NuggetClient> WeaverTest::client;

void WeaverTest::SetUpTestCase() {
  client =
      unique_ptr<nos::NuggetClient>(new nos::NuggetClient(
          nugget_tools::getNosCoreSerial()));
  client->Open();
  EXPECT_TRUE(client->IsOpen()) << "Unable to connect";
}

void WeaverTest::TearDownTestCase() {
  client->Close();
  client = unique_ptr<nos::NuggetClient>();
}

void WeaverTest::testWrite(uint32_t slot, const uint8_t *key,
                           const uint8_t *value) {
  cout << "testWrite " << slot << "\n";
  WriteRequest request;
  WriteResponse response;
  request.set_slot(slot);
  request.set_key(key, KEY_SIZE);
  request.set_value(value, VALUE_SIZE);

  Weaver service(*client);
  ASSERT_NO_ERROR(service.Write(request, &response));
}

void WeaverTest::testRead(uint32_t slot, const uint8_t *key,
                          const uint8_t *value) {
  cout << "testRead " << slot << "\n";
  ReadRequest request;
  ReadResponse response;
  request.set_slot(slot);
  request.set_key(key, KEY_SIZE);

  Weaver service(*client);
  ASSERT_NO_ERROR(service.Read(request, &response));
  ASSERT_EQ(response.error(), ReadResponse::NONE);
  ASSERT_EQ(response.throttle_msec(), 0);
  auto response_value = response.value();
  for (size_t x = 0; x < KEY_SIZE; ++x) {
    ASSERT_EQ(response_value[x], value[x]) << "Inconsistency at index " << x;
  }
}

void WeaverTest::testEraseValue(uint32_t slot) {
  cout << "testEraseValue " << slot << "\n";
  EraseValueRequest request;
  EraseValueResponse response;
  request.set_slot(slot);

  Weaver service(*client);
  ASSERT_NO_ERROR(service.EraseValue(request, &response));
}

void WeaverTest::testReadWrongKey(uint32_t slot, const uint8_t *key,
                                  uint32_t throttle_sec) {
  cout << "testReadWrongKey " << slot << "\n";
  ReadRequest request;
  ReadResponse response;
  request.set_slot(slot);
  request.set_key(key, KEY_SIZE);

  Weaver service(*client);
  ASSERT_NO_ERROR(service.Read(request, &response));
  ASSERT_EQ(response.error(), ReadResponse::WRONG_KEY);
  ASSERT_EQ(response.throttle_msec(), throttle_sec * 1000);
  auto response_value = response.value();
  for (size_t x = 0; x < response_value.size(); ++x) {
    ASSERT_EQ(response_value[x], 0) << "Inconsistency at index " << x;
  }
}

void WeaverTest::testReadThrottle(uint32_t slot, const uint8_t *key,
                                  uint32_t throttle_sec) {
  cout << "testReadThrottle " << slot << "\n";
  ReadRequest request;
  ReadResponse response;
  request.set_slot(slot);
  request.set_key(key, KEY_SIZE);

  Weaver service(*client);
  ASSERT_NO_ERROR(service.Read(request, &response));
  ASSERT_EQ(response.error(), ReadResponse::THROTTLE);
  ASSERT_LE(response.throttle_msec(), throttle_sec * 1000);
  auto response_value = response.value();
  for (size_t x = 0; x < response_value.size(); ++x) {
    ASSERT_EQ(response_value[x], 0) << "Inconsistency at index " << x;
  }
}

TEST_F(WeaverTest, GetConfig) {
  GetConfigRequest request;
  GetConfigResponse response;

  Weaver service(*client);
  ASSERT_NO_ERROR(service.GetConfig(request, &response));
  EXPECT_EQ(response.number_of_slots(), 64);
  EXPECT_EQ(response.key_size(), 16);
  EXPECT_EQ(response.value_size(), 16);
}

TEST_F(WeaverTest, WriteReadErase) {
  const uint8_t ZERO_VALUE[16] = {0};

  testWrite(WeaverTest::slot, TEST_KEY, TEST_VALUE);
  testRead(WeaverTest::slot, TEST_KEY, TEST_VALUE);
  testEraseValue(WeaverTest::slot);

  testRead(WeaverTest::slot, TEST_KEY, ZERO_VALUE);
}

TEST_F(WeaverTest, ReadThrottle) {
  const uint8_t WRONG_KEY[16] = {100, 2, 3, 4, 5, 6, 7, 8,
                                 9, 10, 11, 12, 13, 14, 15, 16};

  testReadWrongKey(WeaverTest::slot, WRONG_KEY, 0);
  testReadWrongKey(WeaverTest::slot, WRONG_KEY, 0);
  testReadWrongKey(WeaverTest::slot, WRONG_KEY, 0);
  testReadWrongKey(WeaverTest::slot, WRONG_KEY, 0);
  testReadWrongKey(WeaverTest::slot, WRONG_KEY, 30);
  testReadThrottle(WeaverTest::slot, WRONG_KEY, 30);
}

TEST_F(WeaverTest, ReadInvalidSlot) {
  ReadRequest request;
  request.set_slot(std::numeric_limits<uint32_t>::max() - 3);
  request.set_key(TEST_KEY, sizeof(TEST_KEY));

  Weaver service(*client);
  ASSERT_EQ(service.Read(request, nullptr), APP_ERROR_BOGUS_ARGS);
}

TEST_F(WeaverTest, WriteInvalidSlot) {
  WriteRequest request;
  request.set_slot(std::numeric_limits<uint32_t>::max() - 5);
  request.set_key(TEST_KEY, sizeof(TEST_KEY));
  request.set_value(TEST_VALUE, sizeof(TEST_VALUE));

  Weaver service(*client);
  ASSERT_EQ(service.Write(request, nullptr), APP_ERROR_BOGUS_ARGS);
}

TEST_F(WeaverTest, EraseValueInvalidSlot) {
  EraseValueRequest request;
  request.set_slot(std::numeric_limits<uint32_t>::max() - 8);

  Weaver service(*client);
  ASSERT_EQ(service.EraseValue(request, nullptr), APP_ERROR_BOGUS_ARGS);
}


}  // namespace
