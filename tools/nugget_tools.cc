#include "nugget_tools.h"

#include "gflags/gflags.h"

DEFINE_string(nos_core_serial, "", "USB device serial number to open");

namespace nugget_tools {

std::string getNosCoreSerial() {
  return FLAGS_nos_core_serial;
}

}  // namespace nugget_tools
