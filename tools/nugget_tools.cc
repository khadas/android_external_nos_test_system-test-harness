#include "nugget_tools.h"

#include <app_nugget.h>

#include <chrono>
#include <thread>
#include <vector>

#ifndef ANDROID
#include "gflags/gflags.h"

DEFINE_string(nos_core_serial, "", "USB device serial number to open");
#endif  // ANDROID

using std::chrono::duration;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using std::chrono::microseconds;

namespace nugget_tools {

std::unique_ptr<nos::NuggetClient> MakeNuggetClient() {
#ifdef ANDROID
  return std::unique_ptr<nos::NuggetClient>(new nos::NuggetClient());
#else
  return std::unique_ptr<nos::NuggetClient>(
      new nos::NuggetClient(FLAGS_nos_core_serial));
#endif
}

bool RebootNugget(nos::NuggetClient *client, uint8_t type) {
  // 0 = soft reboot, 1 = hard reboot
  std::vector<uint8_t> input_buffer(1, type);
  std::vector<uint8_t> output_buffer;
  output_buffer.reserve(sizeof(uint32_t));
  // Capture the time here to allow for some tolerance on the reported time.
  auto start = high_resolution_clock::now();
  if (client->CallApp(APP_ID_NUGGET, NUGGET_PARAM_REBOOT, input_buffer,
                      &output_buffer) != app_status::APP_SUCCESS) {
    return false;
  };

  // Reset the SPI interface.
  client->Close();
  // Time to wait until expecting citadel to be ready after reboot request.
  // This number comes from rebooting citadel on both soft and hard mode to
  // find how much delay is required for Open() to not lock up and tripling it.
  const auto REBOOT_DELAY = std::chrono::microseconds(30000);
  std::this_thread::sleep_for(REBOOT_DELAY);
  client->Open();
  if (!client->IsOpen()) {
    return false;
  }

  // Verify that the monotonic counter a reasonable value.
  if (client->CallApp(APP_ID_NUGGET, NUGGET_PARAM_CYCLES_SINCE_BOOT,
                      input_buffer,
                      &output_buffer) != app_status::APP_SUCCESS) {
    return false;
  };
  if (output_buffer.size() != sizeof(uint32_t)) {
    return false;
  }
  uint32_t post_reboot = *reinterpret_cast<uint32_t *>(output_buffer.data());
  if (std::chrono::microseconds(post_reboot) >
      duration_cast<microseconds>(high_resolution_clock::now() - start)) {
    return false;
  }
  return true;
}

}  // namespace nugget_tools
