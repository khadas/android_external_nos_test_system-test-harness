#ifndef NUGGET_TOOLS_H
#define NUGGET_TOOLS_H

#include <string>
#include <nos/LinuxCitadelClient.h>

#define ASSERT_NO_ERROR(code) \
  ASSERT_EQ(code, app_status::APP_SUCCESS) \
      << code << " is " << nos::NuggetClient::StatusCodeString(code)

namespace nugget_tools {

int32_t getNosCoreFreq();
std::string getNosCoreSerial();

}  // namespace nugget_tools

#endif  // NUGGET_TOOLS_H
