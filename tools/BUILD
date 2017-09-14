cc_binary(
    name = "protoc_gen_nos_client_cpp",
    srcs = [
        "protoc-gen-nos-client-cpp.cc",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":nugget_driver",
        "@com_google_protobuf//:protoc_lib",
        "@nugget_test_systemtestharness_proto//:nugget_protobuf_options_cc_proto",
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
        "@nugget_core_nugget//:driver",
    ],
)
