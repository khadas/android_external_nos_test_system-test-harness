cc_binary(
    name = "protoc-gen-nugget-test-cc",
    srcs = [
        "protoc-gen-nugget-test-cc.cc",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":nugget_driver",
        "@com_google_protobuf//:protoc_lib",
        "@proto//:nugget_protobuf_options_cc_proto",
    ],
)

cc_library(
    name = "nugget_driver",
    srcs = [
        "nugget_driver.cc",
    ],
    hdrs = [
        "nugget_driver.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@com_github_gflags_gflags//:gflags",
        "@nugget//:driver",
    ],
)
