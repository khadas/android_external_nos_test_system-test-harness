#ifndef NUGGET_TOOLS_H
#define NUGGET_TOOLS_H

#include <app_nugget.h>
#include <application.h>
#include <nos/debug.h>
#include <nos/NuggetClient.h>

#include <memory>
#include <string>

#define ASSERT_NO_ERROR(code) \
  ASSERT_EQ(code, app_status::APP_SUCCESS) \
      << code << " is " << nos::StatusCodeString(code)

namespace nugget_tools {

std::unique_ptr<nos::NuggetClient> MakeNuggetClient();

bool RebootNugget(nos::NuggetClient *client, uint8_t type);

}  // namespace nugget_tools

#endif  // NUGGET_TOOLS_H
