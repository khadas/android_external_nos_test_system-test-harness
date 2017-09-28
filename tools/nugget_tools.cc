#include "nugget_tools.h"

#ifndef ANDROID
#include "gflags/gflags.h"

DEFINE_string(nos_core_serial, "", "USB device serial number to open");
#endif  // ANDROID

namespace nugget_tools {

std::unique_ptr<nos::NuggetClient> MakeNuggetClient() {
#ifdef ANDROID
  return std::unique_ptr<nos::NuggetClient>(new nos::NuggetClient());
#else
  return std::unique_ptr<nos::NuggetClient>(
      new nos::NuggetClient(FLAGS_nos_core_serial));
#endif
}

}  // namespace nugget_tools
