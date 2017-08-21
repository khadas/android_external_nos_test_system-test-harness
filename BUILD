COPTS = [
    "-g",
    "-Wall",
    "-Wextra",
]

cc_binary(
    name = "runtests",
    srcs = [
        "src/runtests.cc",
    ],
    deps = [
        ":util",
        "@gtest//:gtest",
        "@gtest//:gtest_main",
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
    deps = [
        ":control_cc_proto",
        "@ahdlc//:ahdlc",
    ],
)

## Proto example
cc_proto_library(
    name = "control_cc_proto",
    deps = [":control_proto"],
)

cc_proto_library(
    name = "diagnostics_api_cc_proto",
    deps = [":diagnostics_api_proto"],
)

################################################################################
# proto libraries
################################################################################

proto_library(
    name = "control_proto",
    srcs = ["proto/control.proto"],
    deps = [
        ":header_proto",
    ],
)

proto_library(
    name = "diagnostics_api_proto",
    srcs = ["proto/diagnostics_api.proto"],
    deps = [
        ":gchips_types_proto",
        ":header_proto",
    ],
)

proto_library(
    name = "gchips_types_proto",
    srcs = ["proto/gchips_types.proto"],
)

proto_library(
    name = "header_proto",
    srcs = ["proto/header.proto"],
)
