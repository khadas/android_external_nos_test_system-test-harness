COPTS = [
    "-std=c++11",
    "-g",
    "-Wall",
    "-Wextra",
]

cc_binary(
    name = "runtests",
    srcs = [
        "src/gtest_with_gflags_main.cc",
        "src/keymaster_tests.cc",
        "src/nugget_core_tests.cc",
        "src/runtests.cc",
        "src/weaver_tests.cc",
    ],
    copts = COPTS,
    deps = [
        ":km_test_lib",
        ":util",
        "@boringssl//:ssl",
        "@com_github_gflags_gflags//:gflags",
        "@gtest//:gtest",
        "@nugget_core_nugget//:config_chip",
        "@nugget_host_generic_libnos//:libnos",
        "@nugget_host_generic_nugget_proto//:keymaster_client_proto",
        "@nugget_host_generic_nugget_proto//:nugget_app_keymaster_keymaster_cc_proto",
        "@nugget_host_generic_nugget_proto//:nugget_app_weaver_weaver_cc_proto",
        "@nugget_host_generic_nugget_proto//:weaver_client_proto",
        "@nugget_host_linux_citadel_libnos_datagram//:libnos_datagram",
        "@nugget_test_systemtestharness_tools//:nugget_tools",
    ],
)

cc_binary(
    name = "cavptests",
    srcs = [
        "src/cavptests.cc",
        "src/gtest_with_gflags_main.cc",
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
        "@nugget_host_generic_libnos//:libnos",
        "@nugget_host_linux_citadel_libnos_datagram//:libnos_datagram",
    ],
)

cc_library(
    name = "util",
    srcs = [
        "src/util.cc",
    ],
    hdrs = [
        "src/blob.h",
        "src/macros.h",
        "src/util.h",
    ],
    copts = COPTS,
    deps = [
        "@com_github_gflags_gflags//:gflags",
        "@nugget_host_generic_nugget_proto//:nugget_app_protoapi_control_cc_proto",
        "@nugget_host_generic_nugget_proto//:nugget_app_protoapi_testing_api_cc_proto",
        "@nugget_test_systemtestharness_tools//:nugget_tools",
        "@nugget_thirdparty_ahdlc//:ahdlc",
    ],
)

cc_library(
    name = "km_test_lib",
    srcs = [
        "src/test-data/test-keys/rsa.cc",
    ],
    hdrs = [
        "src/test-data/test-keys/rsa.h",
    ],
)
