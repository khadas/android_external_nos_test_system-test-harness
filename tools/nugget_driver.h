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

namespace nugget_driver {

extern const uint32_t bufsize;
extern uint8_t *buf;

bool OpenDevice();
void CloseDevice();

}

#endif  // NUGGET_DRIVER_H
