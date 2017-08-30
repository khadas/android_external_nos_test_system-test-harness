#include "nugget_driver.h"

#include "gflags/gflags.h"

/* These are required as globals because of transport.c */
device_t* dev;
int verbose = 0;

//DEFINE_uint32(nos_core_id, 0x00, "App ID (default 0x00)");
//DEFINE_uint32(nos_core_param, 0x00, "Set the command Param field to HEX");
DEFINE_int32(nos_core_freq, 10000000, "SPI clock frequency (default 10MHz)");
DEFINE_string(nos_core_serial, "", "USB device serial number to open");


namespace nugget_driver {

const uint32_t bufsize = 0x4000;
uint8_t buf_imp[bufsize];
uint8_t *buf = buf_imp;

bool OpenDevice() {
  dev = OpenDev(FLAGS_nos_core_freq,
                FLAGS_nos_core_serial.size() ? FLAGS_nos_core_serial.c_str() :
                NULL);
  return dev != NULL;
}

void CloseDevice() {
  CloseDev(dev);
}

}