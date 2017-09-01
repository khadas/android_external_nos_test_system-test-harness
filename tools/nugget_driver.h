#ifndef NUGGET_DRIVER_H
#define NUGGET_DRIVER_H

extern "C" {
#include "core/citadel/config_chip.h"
#include "include/application.h"
#include "include/app_nugget.h"
#include "util/poker/driver.h"
}

extern device_t* dev;
extern int verbose;

#define ASSERT_NO_ERROR(code) \
  ASSERT_EQ(code, app_status::APP_SUCCESS) \
      << code << " is " << nugget_driver::ErrorString(code)

namespace nugget_driver {

extern const uint32_t bufsize;
extern uint8_t *buf;

const char* ErrorString(uint32_t code);

bool OpenDevice();
void CloseDevice();

}  // namespace nugget_driver

#endif  // NUGGET_DRIVER_H
