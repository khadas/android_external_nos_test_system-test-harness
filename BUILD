COPTS = [
    "-g",
    "-Wall",
    "-Wextra",
]

cc_binary(
    name = "runtests",
    srcs = [
        "src/gtest_with_gflags_main.cc",
        "src/nugget_core_tests.cc",
        "src/runtests.cc",
        "src/weaver_tests.cc",
    ],
    copts = COPTS,
    deps = [
        ":util",
        "@boringssl//:ssl",
        "@com_github_gflags_gflags//:gflags",
        "@com_googlesource_android_platform_external_regex_re2//:regex_re2",
        "@gtest//:gtest",
        "@nugget_test_systemtestharness_proto//:weaver_cc_proto",
        "@nugget_test_systemtestharness_proto//:weaver_client_proto",
        "@nugget_test_systemtestharness_tools//:nugget_driver",
    ],
)

cc_binary(
    name = "cavptests",
    srcs = [
        "src/gtest_with_gflags_main.cc",
        "src/cavptests.cc",
        "src/test-data/NIST-CAVP/aes-gcm-cavp.h",
    ],
    copts = COPTS,
    includes = [
        "src/test-data/NIST-CAVP",
    ],
    deps = [
        ":util",
        "@com_github_gflags_gflags//:gflags",
        "@gtest//:gtest",
        "@nugget_test_systemtestharness_tools//:nugget_driver",
    ],
)

cc_library(
    name = "util",
    srcs = [
        "src/util.cc",
    ],
    hdrs = [
        "src/util.h",
    ],
    copts = COPTS,
    deps = [
        "@nugget_test_systemtestharness_proto//:protoapi_control_cc_proto",
        "@nugget_test_systemtestharness_proto//:protoapi_testing_api_cc_proto",
        "@nugget_thirdparty_ahdlc//:ahdlc",
    ],
)
