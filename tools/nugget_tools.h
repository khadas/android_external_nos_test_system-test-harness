#ifndef NUGGET_TOOLS_H
#define NUGGET_TOOLS_H

#include <app_nugget.h>
#include <application.h>
#include <nos/debug.h>
#include <nos/NuggetClientInterface.h>

#include <memory>
#include <string>

#define ASSERT_NO_ERROR(code, msg) \
  do { \
    int value = code; \
    ASSERT_EQ(value, app_status::APP_SUCCESS) \
        << value << " is " << nos::StatusCodeString(value) << msg; \
  } while(0)

namespace nugget_tools {

std::unique_ptr<nos::NuggetClientInterface> MakeNuggetClient();

bool RebootNugget(nos::NuggetClientInterface *client, uint8_t type);

// Returns true if Citadel entered deep sleep
// Passes back an underestimate of the number of seconds waited if so.
bool WaitForSleep(nos::NuggetClientInterface *client, uint32_t *seconds_waited);

bool WipeUserData(nos::NuggetClientInterface *client);

}  // namespace nugget_tools

#endif  // NUGGET_TOOLS_H
