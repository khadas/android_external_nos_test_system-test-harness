COPTS = [
    "-g",
    "-Wall",
    "-Wextra",
]

cc_binary(
    name = "runtests",
    srcs = [
        "src/aes_test.cc",
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
        "@ahdlc//:ahdlc",
    ],
)

## Proto example
# cc_proto_library(
#   name = "person_cc_proto",
#   deps = [":person_proto"],
# )
# proto_library(
#   name = "person_proto",
#   srcs = ["person.proto"],
#   deps = [":address_proto"],
# )
