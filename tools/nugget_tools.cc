#include "nugget_tools.h"

#include <app_nugget.h>
#include <nos/NuggetClient.h>

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#ifdef ANDROID
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

namespace nugget_tools {

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
  return std::unique_ptr<nos::NuggetClientInterface>(
      new nos::NuggetClient(FLAGS_nos_core_serial));
#endif
}

bool RebootNugget(nos::NuggetClientInterface *client, uint8_t type) {
  // 0 = soft reboot, 1 = hard reboot
  std::vector<uint8_t> input_buffer(1, type);
  std::vector<uint8_t> output_buffer;
  output_buffer.reserve(sizeof(uint32_t));
  // Capture the time here to allow for some tolerance on the reported time.
  auto start = high_resolution_clock::now();

  // See what time Nugget OS has now
  if (client->CallApp(APP_ID_NUGGET, NUGGET_PARAM_CYCLES_SINCE_BOOT,
                      input_buffer,
                      &output_buffer) != app_status::APP_SUCCESS) {
    LOG(ERROR) << "CallApp(..., NUGGET_PARAM_CYCLES_SINCE_BOOT, ...) failed!\n";
    return false;
  };
  if (output_buffer.size() != sizeof(uint32_t)) {
    LOG(ERROR) << "Unexpected size of output!\n";
    return false;
  }
  uint32_t pre_reboot = *reinterpret_cast<uint32_t *>(output_buffer.data());

  // Tell it to reboot
  if (client->CallApp(APP_ID_NUGGET, NUGGET_PARAM_REBOOT, input_buffer,
                      &output_buffer) != app_status::APP_SUCCESS) {
    LOG(ERROR) << "CallApp(..., NUGGET_PARAM_REBOOT, ...) failed!\n";
    return false;
  };

  // See what time Nugget OS has after rebooting.
  if (client->CallApp(APP_ID_NUGGET, NUGGET_PARAM_CYCLES_SINCE_BOOT,
                      input_buffer,
                      &output_buffer) != app_status::APP_SUCCESS) {
    LOG(ERROR) << "CallApp(..., NUGGET_PARAM_CYCLES_SINCE_BOOT, ...) failed!\n";
    return false;
  };
  if (output_buffer.size() != sizeof(uint32_t)) {
    LOG(ERROR) << "Unexpected size of output!\n";
    return false;
  }
  uint32_t post_reboot = *reinterpret_cast<uint32_t *>(output_buffer.data());

  // Hard reboots reset the clock to zero, but soft reboots should keep counting
  if (!type) {
    // Make sure time advanced
    if (std::chrono::microseconds(post_reboot) <=
	std::chrono::microseconds(pre_reboot)) {
      LOG(ERROR) << "pre_reboot time (" << pre_reboot << ") should be less than "
                 << "post_reboot time (" << post_reboot << ")\n";
      return false;
    }
    // Change this to elapsed time, not absolute time
    post_reboot -= pre_reboot;
  }

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

}  // namespace nugget_tools
