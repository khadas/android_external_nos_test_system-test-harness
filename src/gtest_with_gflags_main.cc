#ifndef ANDROID
#include "gflags/gflags.h"
#endif  // ANDROID
#include "gmock/gmock.h"
#include "gtest/gtest.h"

int main(int argc, char** argv) {
  testing::InitGoogleMock(&argc, argv);
#ifndef ANDROID
  google::ParseCommandLineFlags(&argc, &argv, true);
#endif  // ANDROID
  return RUN_ALL_TESTS();
}
