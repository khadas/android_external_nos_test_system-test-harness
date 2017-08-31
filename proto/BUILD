################################################################################
# proto cc libraries
################################################################################

cc_proto_library(
    name = "protoapi_control_cc_proto",
    visibility = ["//visibility:public"],
    deps = [
        ":protoapi_control_proto",
    ],
)

cc_proto_library(
    name = "protoapi_diagnostics_api_cc_proto",
    visibility = ["//visibility:public"],
    deps = [
        ":protoapi_diagnostics_api_proto",
    ],
)

cc_proto_library(
    name = "protoapi_testing_api_cc_proto",
    visibility = ["//visibility:public"],
    deps = [
        ":protoapi_testing_api_proto",
    ],
)

cc_proto_library(
    name = "nugget_protobuf_options_cc_proto",
    visibility = ["//visibility:public"],
    deps = [
        "nugget_protobuf_options_proto",
    ],
)

cc_library(
    name = "weaver_cc_proto",
    srcs = [
        "weaver.pb.cc",
    ],
    hdrs = [
        "weaver.pb.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "nugget_protobuf_options_cc_proto",
    ],
)

################################################################################
# proto genrules
################################################################################

genrule(
    name = "weaver_proto_genrule",
    srcs = [
        "weaver.proto",
        "nugget/protobuf/options.proto",
        "google/protobuf/descriptor.proto",
    ],
    outs = [
        "weaver.pb.cc",
        "weaver.pb.h",
    ],
    cmd = "$(location @com_google_protobuf//:protoc) --proto_path=`dirname $(location weaver.proto)` --cpp_out=`dirname $(location weaver.pb.cc)` $(location weaver.proto)",
    tools = [
        "@com_google_protobuf//:protoc",
        "@tools//:protoc-gen-nugget-test-cc",
    ],
)

################################################################################
# proto libraries
################################################################################

proto_library(
    name = "google_protobuf_descriptor_proto",
    srcs = [
        "google/protobuf/descriptor.proto",
    ],
)

proto_library(
    name = "protoapi_control_proto",
    srcs = [
        "protoapi/control.proto",
    ],
    deps = [
        ":protoapi_header_proto",
    ],
)

proto_library(
    name = "protoapi_diagnostics_api_proto",
    srcs = [
        "protoapi/diagnostics_api.proto",
    ],
    deps = [
        ":protoapi_gchips_types_proto",
        ":protoapi_header_proto",
    ],
)

proto_library(
    name = "protoapi_gchips_types_proto",
    srcs = [
        "protoapi/gchips_types.proto",
    ],
)

proto_library(
    name = "protoapi_header_proto",
    srcs = [
        "protoapi/header.proto",
    ],
)

proto_library(
    name = "protoapi_testing_api_proto",
    srcs = [
        "protoapi/testing_api.proto",
    ],
    deps = [
        ":protoapi_header_proto",
    ],
)

proto_library(
    name = "nugget_protobuf_options_proto",
    srcs = [
        "nugget/protobuf/options.proto",
    ],
    deps = [
        ":google_protobuf_descriptor_proto",
    ],
)

proto_library(
    name = "weaver_proto",
    srcs = [
        "weaver.proto",
    ],
    deps = [
        ":nugget_protobuf_options_proto",
    ],
)
