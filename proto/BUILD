################################################################################
# proto cc libraries
################################################################################

cc_proto_library(
    name = "protoapi_control_cc_proto",
    visibility = ["//visibility:public"],
    deps = [
        "protoapi_control_proto",
    ],
)

cc_proto_library(
    name = "protoapi_diagnostics_api_cc_proto",
    visibility = ["//visibility:public"],
    deps = [
        "protoapi_diagnostics_api_proto",
    ],
)

cc_proto_library(
    name = "protoapi_testing_api_cc_proto",
    visibility = ["//visibility:public"],
    deps = [
        "protoapi_testing_api_proto",
    ],
)

cc_proto_library(
    name = "nugget_protobuf_options_cc_proto",
    visibility = ["//visibility:public"],
    deps = [
        "nugget_protobuf_options_proto",
    ],
)

cc_proto_library(
    name = "weaver_cc_proto",
    visibility = ["//visibility:public"],
    deps = [
        "weaver_proto",
    ],
)

cc_library(
    name = "weaver_client_proto",
    srcs = [
        "Weaver.client.cc",
    ],
    hdrs = [
        "Weaver.client.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "nugget_protobuf_options_cc_proto",
        "weaver_cc_proto",
        "@nugget_test_systemtestharness_tools//:nugget_driver",
    ],
)

################################################################################
# proto genrules
################################################################################

GEN_SERVICE = "$(location @com_google_protobuf//:protoc) --plugin=protoc-gen-nos-client-cpp=$(location @nugget_test_systemtestharness_tools//:protoc_gen_nos_client_cpp)"

genrule(
    name = "weaver_proto_client_cc_genrule",
    srcs = [
        "weaver.proto",
        "nugget/protobuf/options.proto",
        "google/protobuf/descriptor.proto",
    ],
    outs = [
        "Weaver.client.cc",
    ],
    cmd = GEN_SERVICE + " --proto_path=`dirname $(location weaver.proto)` --nos-client-cpp_out=source:`dirname $(location Weaver.client.cc)` $(location weaver.proto)",
    tools = [
        "@com_google_protobuf//:protoc",
        "@nugget_test_systemtestharness_tools//:protoc_gen_nos_client_cpp",
    ],
)

genrule(
    name = "weaver_proto_client_h_genrule",
    srcs = [
        "weaver.proto",
        "nugget/protobuf/options.proto",
        "google/protobuf/descriptor.proto",
    ],
    outs = [
        "Weaver.client.h",
    ],
    cmd = GEN_SERVICE + " --proto_path=`dirname $(location weaver.proto)` --nos-client-cpp_out=header:`dirname $(location Weaver.client.h)` $(location weaver.proto)",
    tools = [
        "@com_google_protobuf//:protoc",
        "@nugget_test_systemtestharness_tools//:protoc_gen_nos_client_cpp",
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
        "protoapi_gchips_types_proto",
        "protoapi_header_proto",
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
        "protoapi_header_proto",
    ],
)

proto_library(
    name = "nugget_protobuf_options_proto",
    srcs = [
        "nugget/protobuf/options.proto",
    ],
    deps = [
        "google_protobuf_descriptor_proto",
    ],
)

proto_library(
    name = "weaver_proto",
    srcs = [
        "weaver.proto",
    ],
    deps = [
        "nugget_protobuf_options_proto",
    ],
)
