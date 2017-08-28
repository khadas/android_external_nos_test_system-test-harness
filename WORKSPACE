new_http_archive(
    name = "gtest",
    url = "https://github.com/google/googletest/archive/release-1.8.0.zip",
    build_file = "third_party/BUILD.gtest",
    sha256 = "f3ed3b58511efd272eb074a3a6d6fb79d7c2e6a0e374323d1e6bcbcc1ef141bf",
    strip_prefix = "googletest-release-1.8.0",
)

http_archive(
    name = "com_google_protobuf",
    url = "https://github.com/google/protobuf/archive/v3.3.2.zip",
    strip_prefix = "protobuf-3.3.2",
    sha256 = "c895ad9fd792532f233ced36969d9cc4daec5cb7de9db0d9b26cf06ccd0183c1",
)

http_archive(
    name = "com_google_protobuf_cc",
    url = "https://github.com/google/protobuf/archive/v3.3.2.zip",
    strip_prefix = "protobuf-3.3.2",
    sha256 = "c895ad9fd792532f233ced36969d9cc4daec5cb7de9db0d9b26cf06ccd0183c1",
)

http_archive(
    name = "com_github_gflags_gflags",
    url = "https://github.com/gflags/gflags/archive/v2.2.1.zip",
    strip_prefix = "gflags-2.2.1",
    sha256 = "4e44b69e709c826734dbbbd5208f61888a2faf63f239d73d8ba0011b2dccc97a",
)

## This is the rule for when this repository is outside of repo.
# new_git_repository(
#     name = "ahdlc",
#     remote = "https://nugget-os.googlesource.com/third_party/ahdlc",
# )

## Use this when a subproject of repo.
new_local_repository(
    name = "ahdlc",
    path = "ahdlc",
    build_file_content = """
cc_library(
    name = "ahdlc",
    srcs = [
        "src/lib/crc_16.c",
        "src/lib/frame_layer.c",
    ],
    hdrs = [
        "src/lib/inc/crc_16.h",
        "src/lib/inc/frame_layer.h",
        "src/lib/inc/frame_layer_types.h",
    ],
    visibility = ["//visibility:public"],
)"""
)

new_local_repository(
    name = "libmpsse",
    path = "libmpsse/src",
    build_file_content = """
cc_library(
    name = "libmpsse",
    srcs = [
        "mpsse.c",
        "fast.c",
        "support.c",
    ],
    hdrs = [
        "config.h",
        "mpsse.h",
        "support.h",
    ],
    copts = [
        "-Wall",
        "-fPIC",
        "-fno-strict-aliasing",
        "-g",
        "-O2",
    ],
    linkopts = [
        "-lftdi",
    ],
    visibility = ["//visibility:public"],
)"""
)

new_local_repository(
    name = "nugget",
    path = "nugget",
    build_file_content = """
cc_library(
    name = "driver",
    srcs = [
        "util/poker/driver.c",
        "util/poker/transport.c",
    ],
    hdrs = [
        "core/citadel/config_chip.h",
        "include/application.h",
        "include/app_nugget.h",
        "util/poker/driver.h",
    ],
    deps = [
        "@libmpsse//:libmpsse",
    ],
    visibility = ["//visibility:public"],
)"""
)
