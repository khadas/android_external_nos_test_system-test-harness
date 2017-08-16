COPTS = [
    "-Wall",
    "-Wextra",
]

cc_binary(
  name = "runtests",
  srcs = [
    "src/aes_tests.cc"
  ],
  deps = [
    '@gtest//:gtest',
    '@gtest//:gtest_main',
    '@protobuf//:protobuf'
  ],
)
