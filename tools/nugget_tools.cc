#include "nugget_tools.h"

#include "gflags/gflags.h"

DEFINE_int32(nos_core_freq, 10000000, "SPI clock frequency (default 10MHz)");
DEFINE_string(nos_core_serial, "", "USB device serial number to open");


namespace nugget_tools {

int32_t getNosCoreFreq() {
  return FLAGS_nos_core_freq;
}

std::string getNosCoreSerial() {
  return FLAGS_nos_core_serial;
}

}  // namespace nugget_tools
