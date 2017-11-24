
#include <memory>

#include "gtest/gtest.h"
#include "nugget_tools.h"
#include "nugget/app/avb/avb.pb.h"
#include "Avb.client.h"
#include <avb.h>
#include <application.h>
#include <nos/AppClient.h>
#include <nos/NuggetClientInterface.h>

using std::cout;
using std::string;
using std::unique_ptr;

using namespace nugget::app::avb;

namespace {

class AvbTest: public testing::Test {
 protected:
  static unique_ptr<nos::NuggetClientInterface> client;

  static void SetUpTestCase();
  static void TearDownTestCase();

  virtual void SetUp(void);

  void SetBootloader(void);
  void BootloaderDone(void);

  void GetState(bool *bootloader, bool *production, uint8_t *locks);
  int Reset(ResetRequest_ResetKind kind);

  int Load(uint8_t slot, uint64_t *version);
  int Store(uint8_t slot, uint64_t version);
  int SetDeviceLock(uint8_t locked);
  int SetBootLock(uint8_t locked);
  int SetOwnerLock(uint8_t locked, const uint8_t *metadata, size_t size);
  int GetOwnerKey(uint32_t offset, uint8_t *metadata, size_t *size);
  int SetCarrierLock(uint8_t locked, const uint8_t *metadata, size_t size);
  int SetProduction(bool production);

 public:
  const uint64_t LAST_NONCE = 0x4141414141414140ULL;
  const uint64_t VERSION = 1;
  const uint64_t NONCE = 0x4141414141414141ULL;
  const uint8_t DEVICE_DATA[32] = {
    0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
    0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
    0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
    0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
  };
  const uint8_t SIGNATURE[256] = {
    0x68, 0x86, 0x9a, 0x16, 0xca, 0x62, 0xea, 0xa9,
    0x9b, 0xa0, 0x51, 0x03, 0xa6, 0x00, 0x3f, 0xe8,
    0xf1, 0x43, 0xe6, 0xb7, 0xde, 0x76, 0xfe, 0x21,
    0x65, 0x87, 0x78, 0xe5, 0x1d, 0x11, 0x6a, 0xe1,
    0x7b, 0xc6, 0x2e, 0xe2, 0x96, 0x25, 0x48, 0xa7,
    0x09, 0x43, 0x2c, 0xfd, 0x28, 0xa9, 0x66, 0x8a,
    0x09, 0xd5, 0x83, 0x3b, 0xde, 0x18, 0x5d, 0xef,
    0x50, 0x12, 0x8a, 0x8d, 0xfb, 0x2d, 0x46, 0x20,
    0x69, 0x55, 0x4e, 0x86, 0x63, 0xf6, 0x10, 0xe3,
    0x59, 0x3f, 0x55, 0x72, 0x18, 0xcb, 0x60, 0x80,
    0x0d, 0x2e, 0x2f, 0xfc, 0xc2, 0xbf, 0xda, 0x3f,
    0x4f, 0x2b, 0x6b, 0xf1, 0x5d, 0x28, 0x6b, 0x2b,
    0x9b, 0x92, 0xf3, 0x4e, 0xf2, 0xb6, 0x23, 0x8e,
    0x50, 0x64, 0xf6, 0xee, 0xc7, 0x78, 0x6a, 0xe0,
    0xed, 0xce, 0x2c, 0x1f, 0x0a, 0x47, 0x43, 0x5c,
    0xe4, 0x69, 0xc5, 0xc1, 0xf9, 0x52, 0x8c, 0xed,
    0xfd, 0x71, 0x8f, 0x9a, 0xde, 0x62, 0xfc, 0x21,
    0x07, 0xf9, 0x5f, 0xe1, 0x1e, 0xdc, 0x65, 0x95,
    0x15, 0xc8, 0xe7, 0xf2, 0xce, 0xa9, 0xd0, 0x55,
    0xf1, 0x18, 0x89, 0xae, 0xe8, 0x47, 0xd8, 0x8a,
    0x1f, 0x68, 0xa8, 0x6f, 0x5e, 0x5c, 0xda, 0x3d,
    0x98, 0xeb, 0x82, 0xf8, 0x1f, 0x7a, 0x43, 0x6d,
    0x3a, 0x7c, 0x36, 0x76, 0x4f, 0x55, 0xa4, 0x55,
    0x5f, 0x52, 0x47, 0xa5, 0x71, 0x17, 0x7b, 0x73,
    0xaa, 0x5c, 0x85, 0x94, 0xb6, 0xe2, 0x37, 0x1f,
    0x22, 0x29, 0x46, 0x59, 0x20, 0x1f, 0x49, 0x36,
    0x50, 0xa9, 0x60, 0x5d, 0xeb, 0x99, 0x3f, 0x92,
    0x31, 0xa0, 0x1d, 0xad, 0xdb, 0xde, 0x40, 0xf6,
    0xaf, 0x9c, 0x36, 0xe4, 0x0c, 0xf4, 0xcc, 0xaf,
    0x9f, 0x8b, 0xf9, 0xe6, 0x12, 0x53, 0x4e, 0x58,
    0xeb, 0x9a, 0x57, 0x08, 0x89, 0xa5, 0x4f, 0x7c,
    0xb9, 0x78, 0x07, 0x02, 0x17, 0x2c, 0xce, 0xb8,
  };
};

unique_ptr<nos::NuggetClientInterface> AvbTest::client;

void AvbTest::SetUpTestCase() {
  client = nugget_tools::MakeNuggetClient();
  client->Open();
  EXPECT_TRUE(client->IsOpen()) << "Unable to connect";
}

void AvbTest::TearDownTestCase() {
  client->Close();
  client = unique_ptr<nos::NuggetClientInterface>();
}

void AvbTest::SetUp(void)
{
  bool bootloader;
  bool production;
  uint8_t locks[4];
  int code;

  // Bootloader mode == r00t
  SetBootloader();

  code = SetProduction(false);
  ASSERT_NO_ERROR(code);

  code = Reset(ResetRequest::LOCKS);
  ASSERT_NO_ERROR(code);

  // End of root
  BootloaderDone();

  GetState(&bootloader, &production, locks);
  EXPECT_EQ(bootloader, false);
  EXPECT_EQ(production, false);
  EXPECT_EQ(locks[BOOT], 0x00);
  EXPECT_EQ(locks[CARRIER], 0x00);
  EXPECT_EQ(locks[DEVICE], 0x00);
  EXPECT_EQ(locks[OWNER], 0x00);
}

void AvbTest::SetBootloader(void)
{
  // Force AVB to believe that the AP is in the BIOS.
  ::nos::AppClient app(*client, APP_ID_AVB_TEST);

  /* We have to have a buffer, because it's called by reference. */
  std::vector<uint8_t> buffer;

  // No params, no args needed. This is all that the fake "AVB_TEST" app does.
  uint32_t retval = app.Call(0, buffer, &buffer);

  EXPECT_EQ(retval, APP_SUCCESS);
}

void AvbTest::BootloaderDone(void)
{
  BootloaderDoneRequest request;

  Avb service(*client);
  ASSERT_NO_ERROR(service.BootloaderDone(request, nullptr));
}

void AvbTest::GetState(bool *bootloader, bool *production, uint8_t *locks)
{
  GetStateRequest request;
  GetStateResponse response;

  Avb service(*client);
  ASSERT_NO_ERROR(service.GetState(request, &response));
  EXPECT_EQ(response.number_of_locks(), 4);

  if (bootloader != NULL)
    *bootloader = response.bootloader();
  if (production != NULL)
    *production = response.production();

  auto response_locks = response.locks();
  if (locks != NULL) {
    for (size_t i = 0; i < response_locks.size(); i++)
      locks[i] = response_locks[i];
  }
}

int AvbTest::Reset(ResetRequest_ResetKind kind)
{
  ResetRequest request;

  request.set_kind(kind);

  Avb service(*client);
  return service.Reset(request, nullptr);
}

int AvbTest::Load(uint8_t slot, uint64_t *version)
{
  LoadRequest request;
  LoadResponse response;
  int code;

  request.set_slot(slot);

  Avb service(*client);
  code = service.Load(request, &response);
  if (code == APP_SUCCESS) {
    *version = response.version();
  }

  return code;
}

int AvbTest::Store(uint8_t slot, uint64_t version)
{
  StoreRequest request;

  request.set_slot(slot);
  request.set_version(version);

  Avb service(*client);
  return service.Store(request, nullptr);
}

int AvbTest::SetDeviceLock(uint8_t locked)
{
  SetDeviceLockRequest request;

  request.set_locked(locked);

  Avb service(*client);
  return service.SetDeviceLock(request, nullptr);
}

int AvbTest::SetBootLock(uint8_t locked)
{
  SetBootLockRequest request;

  request.set_locked(locked);

  Avb service(*client);
  return service.SetBootLock(request, nullptr);
}

int AvbTest::SetOwnerLock(uint8_t locked, const uint8_t *metadata, size_t size)
{
  SetOwnerLockRequest request;

  request.set_locked(locked);
  if (metadata != NULL && size > 0) {
    request.set_key(metadata, size);
  }

  Avb service(*client);
  return service.SetOwnerLock(request, nullptr);
}

int AvbTest::GetOwnerKey(uint32_t offset, uint8_t *metadata, size_t *size)
{
  GetOwnerKeyRequest request;
  GetOwnerKeyResponse response;
  size_t i;
  int code;

  request.set_offset(offset);
  if (size != NULL)
    request.set_size(*size);

  Avb service(*client);
  code = service.GetOwnerKey(request, &response);

  if (code == APP_SUCCESS) {
    auto chunk = response.chunk();

    if (metadata != NULL) {
      for (i = 0; i < chunk.size(); i++)
        metadata[i] = chunk[i];
    }
    if (size != NULL)
      *size = chunk.size();
  }

  return code;
}

int AvbTest::SetCarrierLock(uint8_t locked, const uint8_t *metadata, size_t size)
{
  CarrierLockRequest request;

  request.set_locked(locked);
  if (metadata != NULL && size > 0) {
    request.set_device_data(metadata, size);
  }

  Avb service(*client);
  return service.CarrierLock(request, nullptr);
}

int AvbTest::SetProduction(bool production)
{
  SetProductionRequest request;

  request.set_production(production);

  Avb service(*client);
  return service.SetProduction(request, nullptr);
}

// Tests

TEST_F(AvbTest, CarrierLockTest)
{
  uint8_t carrier_data[AVB_METADATA_MAX_SIZE];
  uint8_t locks[4];
  int code;

  // Test we can set and unset the carrier lock in non production mode
  memset(carrier_data, 0, sizeof(carrier_data));

  code = SetCarrierLock(0x12, carrier_data, sizeof(carrier_data));
  ASSERT_NO_ERROR(code);

  GetState(NULL, NULL, locks);
  ASSERT_EQ(locks[CARRIER], 0x12);

  code = SetCarrierLock(0, NULL, 0);
  ASSERT_NO_ERROR(code);

  // Set production mode
  SetBootloader();
  code = SetProduction(true);
  ASSERT_NO_ERROR(code);
  BootloaderDone();

  // Test we cannot set or unset the carrier lock in production mode
  code = SetCarrierLock(0x12, carrier_data, sizeof(carrier_data));
  ASSERT_EQ(code, APP_ERROR_AVB_AUTHORIZATION);

  GetState(NULL, NULL, locks);
  ASSERT_EQ(locks[CARRIER], 0x00);

  code = SetCarrierLock(0, NULL, 0);
  ASSERT_EQ(code, APP_ERROR_AVB_AUTHORIZATION);
}

TEST_F(AvbTest, CarrierUnlockTest)
{
  CarrierLockTestRequest request;
  CarrierLockTestResponse response;
  CarrierUnlock *token;

  token = new CarrierUnlock();
  token->set_nonce(NONCE);
  token->set_version(VERSION);
  token->set_signature(SIGNATURE, sizeof(SIGNATURE));

  request.set_last_nonce(LAST_NONCE);
  request.set_version(VERSION);
  request.set_device_data(DEVICE_DATA, sizeof(DEVICE_DATA));
  request.set_allocated_token(token);

  Avb service(*client);
  ASSERT_NO_ERROR(service.CarrierLockTest(request, &response));

  // The nonce is covered by the signature, so changing it should trip the
  // signature verification
  token->set_nonce(NONCE + 1);
  ASSERT_EQ(service.CarrierLockTest(request, &response), APP_ERROR_AVB_AUTHORIZATION);
}

TEST_F(AvbTest, DeviceLockTest)
{
  bool bootloader;
  bool production;
  uint8_t locks[4];
  int code;

  // Test cannot set the lock
  SetBootloader();
  code = SetProduction(true);
  ASSERT_NO_ERROR(code);

  code = SetDeviceLock(0x12);
  ASSERT_EQ(code, APP_ERROR_AVB_HLOS);

  // Test can set lock
  code = SetProduction(false);
  ASSERT_NO_ERROR(code);

  code = SetDeviceLock(0x34);
  ASSERT_NO_ERROR(code);

  GetState(&bootloader, &production, locks);
  ASSERT_TRUE(bootloader);
  ASSERT_FALSE(production);
  ASSERT_EQ(locks[DEVICE], 0x34);

  // Test cannot set while set
  code = SetDeviceLock(0x56);
  ASSERT_EQ(code, APP_ERROR_AVB_DENIED);

  GetState(NULL, NULL, locks);
  ASSERT_EQ(locks[DEVICE], 0x34);

  // Test can unset
  code = SetDeviceLock(0x00);
  ASSERT_NO_ERROR(code);

  GetState(NULL, NULL, locks);
  ASSERT_EQ(locks[DEVICE], 0x00);
}

TEST_F(AvbTest, BootLockTest)
{
  uint8_t locks[4];
  int code;

  // Test cannot set lock
  code = SetBootLock(0x12);
  ASSERT_EQ(code, APP_ERROR_AVB_BOOTLOADER);

  GetState(NULL, NULL, locks);
  ASSERT_EQ(locks[BOOT], 0x00);

  // Test cannot set lock while carrier set
  SetBootloader();
  code = SetCarrierLock(0x34, NULL, 0);
  ASSERT_NO_ERROR(code);

  code = SetBootLock(0x56);
  ASSERT_EQ(code, APP_ERROR_AVB_DENIED);

  GetState(NULL, NULL, locks);
  ASSERT_EQ(locks[CARRIER], 0x34);
  ASSERT_EQ(locks[BOOT], 0x00);

  // Test cannot set lock while device also set
  code = SetDeviceLock(0x78);
  ASSERT_NO_ERROR(code);

  code = SetBootLock(0x9A);
  ASSERT_EQ(code, APP_ERROR_AVB_DENIED);

  GetState(NULL, NULL, locks);
  ASSERT_EQ(locks[DEVICE], 0x78);
  ASSERT_EQ(locks[BOOT], 0x00);

  // Test cannot set lock while device set
  code = SetCarrierLock(0x00, NULL, 0);
  ASSERT_NO_ERROR(code);

  code = SetBootLock(0xBC);
  ASSERT_EQ(code, APP_ERROR_AVB_DENIED);

  GetState(NULL, NULL, locks);
  ASSERT_EQ(locks[CARRIER], 0x00);
  ASSERT_EQ(locks[DEVICE], 0x78);
  ASSERT_EQ(locks[BOOT], 0x00);

  // Test can set lock
  code = SetDeviceLock(0x00);
  ASSERT_NO_ERROR(code);

  code = SetBootLock(0xDE);
  ASSERT_NO_ERROR(code);

  GetState(NULL, NULL, locks);
  ASSERT_EQ(locks[DEVICE], 0x00);
  ASSERT_EQ(locks[BOOT], 0xDE);
}

TEST_F(AvbTest, OwnerLockTest)
{
  uint8_t owner_key[AVB_METADATA_MAX_SIZE];
  uint8_t chunk[AVB_CHUNK_MAX_SIZE];
  uint8_t locks[4];
  int code;
  size_t i;

  for (i = 0; i < AVB_METADATA_MAX_SIZE; i += 2) {
    owner_key[i + 0] = (i >> 8) & 0xFF;
    owner_key[i + 1] = (i >> 8) & 0xFF;
  }

  // This should fail when boot lock is not set
  code = SetOwnerLock(0x21, owner_key, sizeof(owner_key));
  ASSERT_EQ(code, APP_ERROR_AVB_DENIED);

  // Set the boot lock
  SetBootloader();
  code = SetBootLock(0x43);
  ASSERT_NO_ERROR(code);

  GetState(NULL, NULL, locks);
  ASSERT_EQ(locks[BOOT], 0x43);

  // Try the owner lock again
  code = SetOwnerLock(0x65, owner_key, sizeof(owner_key));
  ASSERT_NO_ERROR(code);

  GetState(NULL, NULL, locks);
  ASSERT_EQ(locks[OWNER], 0x65);

  for (i = 0; i < AVB_METADATA_MAX_SIZE; i += AVB_CHUNK_MAX_SIZE) {
    size_t size = sizeof(chunk);
    size_t j;

    code = GetOwnerKey(i, chunk, &size);
    ASSERT_NO_ERROR(code);
    ASSERT_EQ(size, sizeof(chunk));
    for (j = 0; j < size; j++) {
      ASSERT_EQ(chunk[j], owner_key[i + j]);
    }
  }

  // Test setting the lock while set fails
  code = SetOwnerLock(0x87, owner_key, sizeof(owner_key));
  ASSERT_EQ(code, APP_ERROR_AVB_DENIED);

  GetState(NULL, NULL, locks);
  ASSERT_EQ(locks[OWNER], 0x65);

  // Clear it
  code = SetOwnerLock(0x00, owner_key, sizeof(owner_key));
  ASSERT_NO_ERROR(code);

  GetState(NULL, NULL, locks);
  ASSERT_EQ(locks[OWNER], 0x00);
}

TEST_F(AvbTest, ProductionMode) {
  bool production;
  uint8_t locks[4];
  int code;

  // Check we're not in production mode
  GetState(NULL, &production, NULL);
  ASSERT_FALSE(production);

  // Set some lock values to make sure production doesn't affect them
  SetBootloader();
  code = SetBootLock(0x11);
  ASSERT_NO_ERROR(code);

  code = SetOwnerLock(0x22, NULL, 0);
  ASSERT_NO_ERROR(code);

  code = SetCarrierLock(0x33, NULL, 0);
  ASSERT_NO_ERROR(code);

  code = SetDeviceLock(0x44);
  ASSERT_NO_ERROR(code);

  // Set production mode
  code = SetProduction(true);
  ASSERT_NO_ERROR(code);

  GetState(NULL, &production, locks);
  ASSERT_TRUE(production);
  ASSERT_EQ(locks[BOOT], 0x11);
  ASSERT_EQ(locks[OWNER], 0x22);
  ASSERT_EQ(locks[CARRIER], 0x33);
  ASSERT_EQ(locks[DEVICE], 0x44);

  // Test production cannot be turned off
  BootloaderDone();
  code = SetProduction(false);
  ASSERT_EQ(code, APP_ERROR_AVB_BOOTLOADER);

  // Test production can be turned off in bootloader mode
  SetBootloader();
  code = SetProduction(false);
  ASSERT_NO_ERROR(code);
}

TEST_F(AvbTest, Rollback) {
  uint64_t value = ~0ULL;
  int code, i;

  // Test we cannot change values in normal mode
  code = SetProduction(true);
  ASSERT_NO_ERROR(code);
  for (i = 0; i < 8; i++) {
    code = Store(i, 0xFF00000011223344 + i);
    ASSERT_EQ(code, APP_ERROR_AVB_BOOTLOADER);

    code = Load(i, &value);
    ASSERT_NO_ERROR(code);
    ASSERT_EQ(value, 0x00);
  }

  // Test we can change values in bootloader mode
  SetBootloader();
  for (i = 0; i < 8; i++) {
    code = Store(i, 0xFF00000011223344 + i);
    ASSERT_NO_ERROR(code);

    code = Load(i, &value);
    ASSERT_NO_ERROR(code);
    ASSERT_EQ(value, 0xFF00000011223344 + i);

    code = Store(i, 0x8800000011223344 - i);
    ASSERT_NO_ERROR(code);

    code = Load(i, &value);
    ASSERT_NO_ERROR(code);
    ASSERT_EQ(value, 0x8800000011223344 - i);
  }
}

TEST_F(AvbTest, Reset)
{
  bool bootloader;
  bool production;
  uint8_t locks[4];
  int code;

  // Set some locks and production mode*/
  SetBootloader();
  code = SetBootLock(0x12);
  ASSERT_NO_ERROR(code);

  code = SetDeviceLock(0x34);
  ASSERT_NO_ERROR(code);

  code = SetProduction(true);
  ASSERT_NO_ERROR(code);

  BootloaderDone();

  GetState(&bootloader, &production, locks);
  ASSERT_FALSE(bootloader);
  ASSERT_TRUE(production);
  ASSERT_EQ(locks[BOOT], 0x12);
  ASSERT_EQ(locks[DEVICE], 0x34);

  // Try reset, should fail
  code = Reset(ResetRequest::LOCKS);
  ASSERT_EQ(code, APP_ERROR_AVB_DENIED);

  GetState(&bootloader, &production, locks);
  ASSERT_FALSE(bootloader);
  ASSERT_TRUE(production);
  ASSERT_EQ(locks[BOOT], 0x12);
  ASSERT_EQ(locks[DEVICE], 0x34);

  // Disable production, try reset, should pass
  SetBootloader();

  code = SetProduction(false);
  ASSERT_NO_ERROR(code);

  BootloaderDone();

  code = Reset(ResetRequest::LOCKS);
  ASSERT_NO_ERROR(code);

  GetState(&bootloader, &production, locks);
  ASSERT_FALSE(bootloader);
  ASSERT_FALSE(production);
  ASSERT_EQ(locks[BOOT], 0x00);
  ASSERT_EQ(locks[DEVICE], 0x00);
}

}  // namespace
