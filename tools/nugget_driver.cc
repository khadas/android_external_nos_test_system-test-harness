#include "nugget_driver.h"

#include "gflags/gflags.h"

/* These are required as globals because of transport.c */
device_t* dev = NULL;
int verbose = 0;

//DEFINE_uint32(nos_core_id, 0x00, "App ID (default 0x00)");
//DEFINE_uint32(nos_core_param, 0x00, "Set the command Param field to HEX");
DEFINE_int32(nos_core_freq, 10000000, "SPI clock frequency (default 10MHz)");
DEFINE_string(nos_core_serial, "", "USB device serial number to open");


namespace nugget_driver {

const uint32_t bufsize = 0x4000;
uint8_t buf_imp[bufsize];
uint8_t *buf = buf_imp;

#define ErrorString_helper(x) \
    case app_status::x: \
      return #x;

const char* ErrorString(uint32_t code) {
  switch (code) {
    ErrorString_helper(APP_SUCCESS)
    ErrorString_helper(APP_ERROR_BOGUS_ARGS)
    ErrorString_helper(APP_ERROR_INTERNAL)
    ErrorString_helper(APP_ERROR_TOO_MUCH)
    default:
      return "unknown";
  }
}

bool OpenDevice() {
  if (dev == NULL) {
    dev = OpenDev(FLAGS_nos_core_freq,
                  FLAGS_nos_core_serial.size() ? FLAGS_nos_core_serial.c_str() :
                  NULL);
  }
  return dev != NULL;
}

void CloseDevice() {
  CloseDev(dev);
  dev = NULL;
}

}  // namespace nugget_driver
