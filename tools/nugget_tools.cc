#include "nugget_tools.h"

#include <app_nugget.h>
#include <nos/NuggetClient.h>

#include <chrono>
#include <cinttypes>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>

#ifdef ANDROID
#include <android-base/endian.h>
#include "nos/CitadeldProxyClient.h"
#else
#include "gflags/gflags.h"

DEFINE_string(nos_core_serial, "", "USB device serial number to open");
#endif  // ANDROID

#ifndef LOG
#define LOG(x) std::cerr << __FILE__ << ":" << __LINE__ << " " << #x << ": "
#endif  // LOG

using std::chrono::duration;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using std::chrono::microseconds;
using std::string;

namespace nugget_tools {

namespace {

void WaitForHardReboot() {
  // POST (which takes ~50ms) runs on a hard-reboot, plus an
  // additional ~30ms for RO+RW verification.
  std::this_thread::sleep_for(std::chrono::milliseconds(80));
}

} // namesapce

std::unique_ptr<nos::NuggetClientInterface> MakeNuggetClient() {
#ifdef ANDROID
  std::unique_ptr<nos::NuggetClientInterface> client =
      std::unique_ptr<nos::NuggetClientInterface>(new nos::NuggetClient());
  client->Open();
  if (!client->IsOpen()) {
    client = std::unique_ptr<nos::NuggetClientInterface>(
        new nos::CitadeldProxyClient());
  }
  return client;
#else
  if (FLAGS_nos_core_serial.empty()) {
    const char *env_default = secure_getenv("CITADEL_DEVICE");
    if (env_default && *env_default) {
      FLAGS_nos_core_serial.assign(env_default);
      std::cerr << "Using CITADEL_DEVICE=" << FLAGS_nos_core_serial << "\n";
    }
  }
  return std::unique_ptr<nos::NuggetClientInterface>(
      new nos::NuggetClient(FLAGS_nos_core_serial));
#endif
}

bool CyclesSinceBoot(nos::NuggetClientInterface *client, uint32_t *cycles) {
  std::vector<uint8_t> buffer;
  buffer.reserve(sizeof(uint32_t));
  if (client->CallApp(APP_ID_NUGGET, NUGGET_PARAM_CYCLES_SINCE_BOOT,
                      buffer, &buffer) != app_status::APP_SUCCESS) {
    perror("test");
    LOG(ERROR) << "CallApp(..., NUGGET_PARAM_CYCLES_SINCE_BOOT, ...) failed!\n";
    return false;
  };
  if (buffer.size() != sizeof(uint32_t)) {
    LOG(ERROR) << "Unexpected size of cycle count!\n";
    return false;
  }
  *cycles = le32toh(*reinterpret_cast<uint32_t *>(buffer.data()));
  return true;
}

bool RebootNugget(nos::NuggetClientInterface *client) {
  // Capture the time here to allow for some tolerance on the reported time.
  auto start = high_resolution_clock::now();

  // See what time Nugget OS has now
  uint32_t pre_reboot;
  if (!CyclesSinceBoot(client, &pre_reboot)) {
    return false;
  }

  // Tell it to reboot: 0 = soft reboot, 1 = hard reboot
  std::vector<uint8_t> input_buffer;
  if (client->CallApp(APP_ID_NUGGET, NUGGET_PARAM_REBOOT, input_buffer,
                      nullptr) != app_status::APP_SUCCESS) {
    LOG(ERROR) << "CallApp(..., NUGGET_PARAM_REBOOT, ...) failed!\n";
    return false;
  }

  WaitForHardReboot();

  // See what time Nugget OS has after rebooting.
  uint32_t post_reboot;
  if (!CyclesSinceBoot(client, &post_reboot)) {
    return false;
  }

  // Hard reboots reset the clock to zero
  // Verify that the Nugget OS counter shows a reasonable value.
  // Use the elapsed time +5% for the threshold.
  auto threshold_microseconds =
      duration_cast<microseconds>(high_resolution_clock::now() - start) *
          105 / 100;
  if (std::chrono::microseconds(post_reboot) > threshold_microseconds ) {
    LOG(ERROR) << "Counter is " << post_reboot
               << " but is expected to be less than "
               << threshold_microseconds.count() * 1.05 << "!\n";
    return false;
  }

  // Looks okay
  return true;
}

bool WaitForSleep(nos::NuggetClientInterface *client, uint32_t *seconds_waited) {
  struct nugget_app_low_power_stats stats0;
  struct nugget_app_low_power_stats stats1;
  std::vector<uint8_t> buffer;

  buffer.reserve(sizeof(struct nugget_app_low_power_stats));
  // Grab stats before sleeping
  if (client->CallApp(APP_ID_NUGGET, NUGGET_PARAM_GET_LOW_POWER_STATS,
                      buffer, &buffer) != app_status::APP_SUCCESS) {
    LOG(ERROR) << "CallApp(..., NUGGET_PARAM_GET_LOW_POWER_STATS, ...) failed!\n";
    return false;
  }
  memcpy(&stats0, buffer.data(), sizeof(stats0));

  // Wait for Citadel to fall asleep
  constexpr uint32_t wait_seconds = 3;
  std::this_thread::sleep_for(std::chrono::seconds(wait_seconds));

  // Grab stats after sleeping
  buffer.empty();
  buffer.reserve(sizeof(struct nugget_app_low_power_stats));
  if (client->CallApp(APP_ID_NUGGET, NUGGET_PARAM_GET_LOW_POWER_STATS,
                      buffer, &buffer) != app_status::APP_SUCCESS) {
    LOG(ERROR) << "CallApp(..., NUGGET_PARAM_GET_LOW_POWER_STATS, ...) failed!\n";
    return false;
  }
  memcpy(&stats1, buffer.data(), sizeof(stats1));

  // Verify that Citadel went to sleep but didn't reboot
  if (stats1.hard_reset_count == stats0.hard_reset_count &&
      stats1.deep_sleep_count == stats0.deep_sleep_count + 1 &&
      stats1.wake_count == stats0.wake_count +1 &&
      stats1.time_spent_in_deep_sleep > stats0.time_spent_in_deep_sleep) {
    // Yep, looks good
    if (seconds_waited) {
      *seconds_waited = wait_seconds;
    }
    return true;
  }

  LOG(ERROR) << "Citadel didn't go to sleep as expected\n";
  return false;
}

bool WipeUserData(nos::NuggetClientInterface *client) {
  // Request wipe of user data which should hard reboot
  std::vector<uint8_t> buffer(4);
  *reinterpret_cast<uint32_t *>(buffer.data()) = htole32(ERASE_CONFIRMATION);
  if (client->CallApp(APP_ID_NUGGET, NUGGET_PARAM_NUKE_FROM_ORBIT,
                         buffer, nullptr) != app_status::APP_SUCCESS) {
    return false;
  }
  WaitForHardReboot();
  return true;
}

}  // namespace nugget_tools
